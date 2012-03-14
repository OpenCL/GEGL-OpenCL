/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007, 2008 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <gio/gio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-file.h"
#include "gegl-buffer-index.h"
#include "gegl-buffer-types.h"
#include "gegl-debug.h"
//#include "gegl-types-internal.h"


#ifndef HAVE_FSYNC

#ifdef G_OS_WIN32
#define fsync _commit
#endif

#endif

struct _GeglTileBackendFile
{
  GeglTileBackend  parent_instance;

  /* the path to our buffer */
  gchar           *path;

  /* the file exist (and we've thus been able to initialize i and o,
   * the utility call ensure_exist() should be called before any code
   * using i and o)
   */
  gboolean         exist;

  /* total size of file */
  guint            total;

  /* hashtable containing all entries of buffer, the index is written
   * to the swapfile conforming to the structures laid out in
   * gegl-buffer-index.h
   */
  GHashTable      *index;

  /* list of offsets to tiles that are free */
  GSList          *free_list;

  /* offset to next pre allocated tile slot */
  guint            next_pre_alloc;

  /* revision of last index sync, for cooperated sharing of a buffer
   * file
   */
  guint32          rev;

  /* a local copy of the header that will be written to the file, in a
   * multiple user per buffer scenario, the flags in the header might
   * be used for locking/signalling
   */
  GeglBufferHeader header;

  gint            foffset;

  /* current offset, used when writing the index */
  gint             offset;

  /* when writing buffer blocks the writer keeps one block unwritten
   * at all times to be able to keep track of the ->next offsets in
   * the blocks.
   */
  GeglBufferBlock *in_holding;

  /* loading buffer */
  GList           *tiles;

  /* GFile refering to our buffer */
  GFile           *file;

  GFileMonitor    *monitor;
  /* for writing */
  int              o;

  /* for reading */
  int              i;
};


static void     gegl_tile_backend_file_ensure_exist (GeglTileBackendFile *self);
static gboolean gegl_tile_backend_file_write_block  (GeglTileBackendFile *self,
                                                     GeglBufferBlock     *block);
static void     gegl_tile_backend_file_dbg_alloc    (int                  size);
static void     gegl_tile_backend_file_dbg_dealloc  (int                  size);


static inline void
gegl_tile_backend_file_file_entry_read (GeglTileBackendFile *self,
                                        GeglBufferTile      *entry,
                                        guchar              *dest)
{
  gint     to_be_read;
  gboolean success;
  gint     tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  goffset  offset = entry->offset;
  guchar  *tdest = dest;

  gegl_tile_backend_file_ensure_exist (self);

  if (self->foffset != offset)
    {
      success = (lseek (self->i, offset, SEEK_SET) >= 0);

      if (success == FALSE)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      self->foffset = offset;
    }
  to_be_read = tile_size;

  while (to_be_read > 0)
    {
      GError *error = NULL;
      gint byte_read;

      byte_read = read (self->i, tdest + tile_size - to_be_read, to_be_read);
      if (byte_read <= 0)
        {
          g_message ("unable to read tile data from self: "
                     "%s (%d/%d bytes read) %s",
                     g_strerror (errno), byte_read, to_be_read, error?error->message:"--");
          return;
        }
      to_be_read -= byte_read;
      self->foffset += byte_read;
    }

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i at %i", entry->x, entry->y, entry->z, (gint)offset);
}

static inline void
gegl_tile_backend_file_file_entry_write (GeglTileBackendFile *self,
                                         GeglBufferTile      *entry,
                                         guchar              *source)
{
  gint     to_be_written;
  gboolean success;
  goffset  offset = entry->offset;
  gint     tile_size;

  gegl_tile_backend_file_ensure_exist (self);

  if (self->foffset != offset)
    {
      success = (lseek (self->o, offset, SEEK_SET) >= 0);
      if (success == FALSE)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      self->foffset = offset;
    }

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));

  to_be_written = tile_size;

  while (to_be_written > 0)
    {
      gint wrote;
      wrote = write (self->o,
                     source + tile_size - to_be_written,
                     to_be_written);
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          return;
        }
      to_be_written -= wrote;
      self->foffset += wrote;
    }
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "wrote entry %i,%i,%i at %i", entry->x, entry->y, entry->z, (gint)offset);
}

static inline GeglBufferTile *
gegl_tile_backend_file_file_entry_new (GeglTileBackendFile *self)
{
  GeglBufferTile *entry = gegl_tile_entry_new (0,0,0);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Creating new entry");

  gegl_tile_backend_file_ensure_exist (self);

  if (self->free_list)
    {
      /* XXX: losing precision ?
       * the free list seems to operate with fixed size datums and
       * only keep track of offsets.
       */
      gint offset = GPOINTER_TO_INT (self->free_list->data);
      entry->offset = offset;
      self->free_list = g_slist_remove (self->free_list, self->free_list->data);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i from free list", ((gint)entry->offset));
    }
  else
    {
      gint tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));

      entry->offset = self->next_pre_alloc;
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i (next allocation)", (gint)entry->offset);
      self->next_pre_alloc += tile_size;

      if (self->next_pre_alloc >= self->total) /* automatic growing ensuring that
                                                  we have room for next allocation..
                                                */
        {
          self->total = self->total + 32 * tile_size;

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "growing file to %i bytes", (gint)self->total);

          ftruncate (self->o, self->total);
          self->foffset = -1;
        }
    }
  gegl_tile_backend_file_dbg_alloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
  return entry;
}

static inline void
gegl_tile_backend_file_file_entry_destroy (GeglBufferTile      *entry,
                                           GeglTileBackendFile *self)
{
  /* XXX: EEEk, throwing away bits */
  guint offset = entry->offset;
  self->free_list = g_slist_prepend (self->free_list,
                                     GUINT_TO_POINTER (offset));
  g_hash_table_remove (self->index, entry);

  gegl_tile_backend_file_dbg_dealloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
  g_free (entry);
}

static gboolean
gegl_tile_backend_file_write_header (GeglTileBackendFile *self)
{
  gboolean success;

  gegl_tile_backend_file_ensure_exist (self);

  success = (lseek (self->o, 0, SEEK_SET) != -1);
  if (success == FALSE)
    {
      g_warning ("unable to seek in buffer");
      return FALSE;
    }
  write (self->o, &(self->header), 256);
  self->foffset = 256;
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Wrote header, next=%i", (gint)self->header.next);
  return TRUE;
}

static gboolean
gegl_tile_backend_file_write_block (GeglTileBackendFile *self,
                                    GeglBufferBlock     *block)
{
  gegl_tile_backend_file_ensure_exist (self);
  if (self->in_holding)
    {
      guint64 next_allocation = self->offset + self->in_holding->length;


      /* update the next offset pointer in the previous block */
      if (block == NULL)
          /* the previous block was the last block */
          self->in_holding->next = 0;
      else
          self->in_holding->next = next_allocation;

      if (self->foffset != self->offset)
      {
        if(lseek (self->o, self->offset, G_SEEK_SET) == -1)
          goto fail;

        self->foffset = self->offset;
      }

      /* XXX: should promiscuosuly try to compress here as well,. if revisions
              are not matching..
       */

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Wrote block: length:%i flags:%i next:%i at offset %i",
                 self->in_holding->length,
                 self->in_holding->flags,
                 (gint)self->in_holding->next,
                 (gint)self->offset);
      {
        ssize_t written = write (self->o, self->in_holding,
                                 self->in_holding->length);
        if(written != -1)
          {
            self->offset += written;
            self->foffset += written;
          }
      }

      g_assert (next_allocation == self->offset); /* true as long as
                                                     the simple allocation
                                                     scheme is used */

      self->offset = next_allocation;
    }
  else
    {
      /* we're setting up for the first write */

      self->offset = self->next_pre_alloc; /* start writing header at end
                                            * of file, worry about writing
                                            * header inside free list later
                                            */
      if (self->foffset != self->offset)
      {
        if(lseek (self->o, self->offset, G_SEEK_SET) == -1)
          goto fail;

        self->foffset = self->offset;
      }
    }
  self->in_holding = block;

  return TRUE;
fail:
  g_warning ("gegl buffer index writing problems for %s",
             self->path);
  return FALSE;
}

G_DEFINE_TYPE (GeglTileBackendFile, gegl_tile_backend_file, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;

/* this debugging is across all buffers */

static gint allocs         = 0;
static gint file_size      = 0;
static gint peak_allocs    = 0;
static gint peak_file_size = 0;

void
gegl_tile_backend_file_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, file_size / 1024 / 1024.0,
             peak_allocs, peak_file_size, peak_file_size / 1024 / 1024.0);
}

static void
gegl_tile_backend_file_dbg_alloc (gint size)
{
  allocs++;
  file_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (file_size > peak_file_size)
    peak_file_size = file_size;
}

static void
gegl_tile_backend_file_dbg_dealloc (gint size)
{
  allocs--;
  file_size -= size;
}

static inline GeglBufferTile *
gegl_tile_backend_file_lookup_entry (GeglTileBackendFile *self,
              gint                 x,
              gint                 y,
              gint                 z)
{
  GeglBufferTile *ret;
  GeglBufferTile *key = gegl_tile_entry_new (x,y,z);
  ret = g_hash_table_lookup (self->index, key);
  gegl_tile_entry_destroy (key);
  return ret;
}

/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
gegl_tile_backend_file_get_tile (GeglTileSource *self,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  GeglBufferTile      *entry;
  GeglTile            *tile = NULL;
  gint                 tile_size;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (!entry)
    return NULL;

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  tile      = gegl_tile_new (tile_size);
  gegl_tile_set_rev (tile, entry->rev);
  gegl_tile_mark_as_stored (tile);

  gegl_tile_backend_file_file_entry_read (tile_backend_file, entry, gegl_tile_get_data (tile));
  return tile;
}

static gpointer
gegl_tile_backend_file_set_tile (GeglTileSource *self,
                                 GeglTile       *tile,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  GeglBufferTile      *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (entry == NULL)
    {
      entry    = gegl_tile_backend_file_file_entry_new (tile_backend_file);
      entry->x = x;
      entry->y = y;
      entry->z = z;
      g_hash_table_insert (tile_backend_file->index, entry, entry);
    }
  entry->rev = gegl_tile_get_rev (tile);

  gegl_tile_backend_file_file_entry_write (tile_backend_file, entry, gegl_tile_get_data (tile));
  gegl_tile_mark_as_stored (tile);
  return NULL;
}

static gpointer
gegl_tile_backend_file_void_tile (GeglTileSource *self,
                                  GeglTile       *tile,
                                  gint            x,
                                  gint            y,
                                  gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  GeglBufferTile      *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (entry != NULL)
    {
      gegl_tile_backend_file_file_entry_destroy (entry, tile_backend_file);
    }

  return NULL;
}

static gpointer
gegl_tile_backend_file_exist_tile (GeglTileSource *self,
                                   GeglTile       *tile,
                                   gint            x,
                                   gint            y,
                                   gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  GeglBufferTile      *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  return entry!=NULL?((gpointer)0x1):NULL;
}


static gpointer
gegl_tile_backend_file_flush (GeglTileSource *source,
                              GeglTile       *tile,
                              gint            x,
                              gint            y,
                              gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *self;
  GList               *tiles;

  backend  = GEGL_TILE_BACKEND (source);
  self     = GEGL_TILE_BACKEND_FILE (backend);

  gegl_tile_backend_file_ensure_exist (self);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "flushing %s", self->path);


  self->header.rev ++;
  self->header.next = self->next_pre_alloc; /* this is the offset
                                               we start handing
                                               out headers from*/
  tiles = g_hash_table_get_keys (self->index);

  if (tiles == NULL)
    self->header.next = 0;
  else
    {
      GList *iter;
      for (iter = tiles; iter; iter = iter->next)
        {
          GeglBufferItem *item = iter->data;

          gegl_tile_backend_file_write_block (self, &item->block);
        }
      gegl_tile_backend_file_write_block (self, NULL); /* terminate the index */
      g_list_free (tiles);
    }

  gegl_tile_backend_file_write_header (self);
  fsync (self->o);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "flushed %s", self->path);

  return (gpointer)0xf0f;
}

enum
{
  PROP_0,
  PROP_PATH
};

static gpointer
gegl_tile_backend_file_command (GeglTileSource  *self,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return gegl_tile_backend_file_get_tile (self, x, y, z);
      case GEGL_TILE_SET:
        return gegl_tile_backend_file_set_tile (self, data, x, y, z);

      case GEGL_TILE_IDLE:
        return NULL;       /* we could perhaps lazily be writing indexes
                            * at some intervals, making it work as an
                            * autosave for the buffer?
                            */

      case GEGL_TILE_VOID:
        return gegl_tile_backend_file_void_tile (self, data, x, y, z);

      case GEGL_TILE_EXIST:
        return gegl_tile_backend_file_exist_tile (self, data, x, y, z);
      case GEGL_TILE_FLUSH:
        return gegl_tile_backend_file_flush (self, data, x, y, z);

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return FALSE;
}

static void
gegl_tile_backend_file_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);

  switch (property_id)
    {
      case PROP_PATH:
        if (self->path)
          g_free (self->path);
        self->path = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gegl_tile_backend_file_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);

  switch (property_id)
    {
      case PROP_PATH:
        g_value_set_string (value, self->path);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gegl_tile_backend_file_finalize (GObject *object)
{
  GeglTileBackendFile *self = (GeglTileBackendFile *) object;

  if (self->index)
    g_hash_table_unref (self->index);

  if (self->exist)
    {
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "finalizing buffer %s", self->path);

      if (self->i != -1)
        {
          close (self->i);
          self->i = -1;
        }
      if (self->o != -1)
        {
          close (self->o);
          self->o = -1;
        }
    }

  if (self->path)
    g_free (self->path);

  if (self->monitor)
    g_object_unref (self->monitor);

  if (self->file)
    g_object_unref (self->file);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static guint
gegl_tile_backend_file_hashfunc (gconstpointer key)
{
  const GeglBufferTile *e = key;
  guint            hash;
  gint             i;
  gint             srcA = e->x;
  gint             srcB = e->y;
  gint             srcC = e->z;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  hash = 0;
  for (i = 9; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; } while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return hash;
}

static gboolean
gegl_tile_backend_file_equalfunc (gconstpointer a,
           gconstpointer b)
{
  const GeglBufferTile *ea = a;
  const GeglBufferTile *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}


static void
gegl_tile_backend_file_load_index (GeglTileBackendFile *self,
                                   gboolean             block)
{
  GeglBufferHeader new_header;
  GList           *iter;
  GeglTileBackend *backend;
  goffset offset = 0;
  goffset max=0;
  gint tile_size;

  /* compute total from and next pre alloc by monitoring tiles as they
   * are added here
   */
  /* reload header */
  new_header = gegl_buffer_read_header (self->i, &offset)->header;
  self->foffset = 256;

  while (new_header.flags & GEGL_FLAG_LOCKED)
    {
      g_usleep (50000);
      new_header = gegl_buffer_read_header (self->i, &offset)->header;
      self->foffset = 256;
    }

  if (new_header.rev == self->header.rev)
    {
      GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "header not changed: %s", self->path);
      return;
    }
  else
    {
      self->header=new_header;
      GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "loading index: %s", self->path);
    }

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  offset      = self->header.next;
  self->tiles = gegl_buffer_read_index (self->i, &offset);
  self->foffset = -1;
  backend     = GEGL_TILE_BACKEND (self);

  for (iter = self->tiles; iter; iter=iter->next)
    {
      GeglBufferItem *item = iter->data;

      GeglBufferItem *existing = g_hash_table_lookup (self->index, item);

      if (item->tile.offset > max)
        max = item->tile.offset + tile_size;

      if (existing)
        {
          if (existing->tile.rev == item->tile.rev)
            {
              g_assert (existing->tile.offset == item->tile.offset);
              existing->tile = item->tile;
              g_free (item);
              continue;
            }
          else
            {
              GeglTileStorage *storage =
                (void*)gegl_tile_backend_peek_storage (backend);
              GeglRectangle rect;
              g_hash_table_remove (self->index, existing);

              gegl_tile_source_refetch (GEGL_TILE_SOURCE (storage),
                                        existing->tile.x,
                                        existing->tile.y,
                                        existing->tile.z);

              if (existing->tile.z == 0)
                {
                  rect.width = self->header.tile_width;
                  rect.height = self->header.tile_height;
                  rect.x = existing->tile.x * self->header.tile_width;
                  rect.y = existing->tile.y * self->header.tile_height;
                }
              g_free (existing);

              g_signal_emit_by_name (storage, "changed", &rect, NULL);
            }
        }
      g_hash_table_insert (self->index, iter->data, iter->data);
    }
  g_list_free (self->tiles);
  g_slist_free (self->free_list);
  self->free_list      = NULL;
  self->next_pre_alloc = max; /* if bigger than own? */
  self->total          = max;
  self->tiles          = NULL;
}

static void
gegl_tile_backend_file_file_changed (GFileMonitor        *monitor,
              GFile               *file,
              GFile               *other_file,
              GFileMonitorEvent    event_type,
              GeglTileBackendFile *self)
{
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED /*G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT*/ )
    {
      gegl_tile_backend_file_load_index (self, TRUE);
      self->foffset = -1;
    }
}

static GObject *
gegl_tile_backend_file_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GeglTileBackendFile *self;
  GeglTileBackend     *backend;

  object  = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self    = GEGL_TILE_BACKEND_FILE (object);
  backend = GEGL_TILE_BACKEND (object);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing file backend: %s", self->path);

  self->file = g_file_new_for_commandline_arg (self->path);
  self->i = self->o = -1;
  self->index = g_hash_table_new (gegl_tile_backend_file_hashfunc, gegl_tile_backend_file_equalfunc);


  /* If the file already exists open it, assuming it is a GeglBuffer. */
  if (access (self->path, F_OK) != -1)
    {
      goffset offset = 0;

      /* Install a monitor for changes to the file in case other applications
       * might be writing to the buffer
       */
      self->monitor = g_file_monitor_file (self->file, G_FILE_MONITOR_NONE,
                                           NULL, NULL);
      g_signal_connect (self->monitor, "changed",
                        G_CALLBACK (gegl_tile_backend_file_file_changed),
                        self);

      self->o = open (self->path, O_RDWR|O_CREAT, 0770);
      if (self->o == -1)
        {
          /* Try again but this time with only read access. This is
           * a quick-fix for make distcheck, where img_cmp fails
           * when it opens a GeglBuffer file in the source tree
           * (which is read-only).
           */
          self->o = open (self->path, O_RDONLY, 0770);

          if (self->o == -1)
            g_warning ("%s: Could not open '%s': %s", G_STRFUNC, self->path, g_strerror (errno));
        }
      self->i = dup (self->o);

      self->header = gegl_buffer_read_header (self->i, &offset)->header;
      self->header.rev = self->header.rev -1;

      /* we are overriding all of the work of the actual constructor here,
       * a really evil hack :d
       */
      backend->priv->tile_width = self->header.tile_width;
      backend->priv->tile_height = self->header.tile_height;
      backend->priv->format = babl_format (self->header.description);
      backend->priv->px_size = babl_format_get_bytes_per_pixel (backend->priv->format);
      backend->priv->tile_size = backend->priv->tile_width *
                                    backend->priv->tile_height *
                                    backend->priv->px_size;

      /* insert each of the entries into the hash table */
      gegl_tile_backend_file_load_index (self, TRUE);
      self->exist = TRUE;
      g_assert (self->i != -1);
      g_assert (self->o != -1);

      /* to autoflush gegl_buffer_set */

      /* XXX: poking at internals, icky */
      backend->priv->shared = TRUE;
    }
  else
    {
      self->exist = FALSE; /* this is also the default, the file will be created on demand */
    }

  g_assert (self->file);

  backend->priv->header = &self->header;

  return object;
}

static void
gegl_tile_backend_file_ensure_exist (GeglTileBackendFile *self)
{
  if (!self->exist)
    {
      GeglTileBackend *backend;
      self->exist = TRUE;

      backend = GEGL_TILE_BACKEND (self);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "creating swapfile  %s", self->path);

      self->o = open (self->path, O_RDWR|O_CREAT, 0770);
      if (self->o == -1)
        g_warning ("%s: Could not open '%s': %s", G_STRFUNC, self->path, g_strerror (errno));

      self->next_pre_alloc = 256;  /* reserved space for header */
      self->total          = 256;  /* reserved space for header */
      self->foffset        = 0;
      gegl_buffer_header_init (&self->header,
                               backend->priv->tile_width,
                               backend->priv->tile_height,
                               backend->priv->px_size,
                               backend->priv->format
                               );
      gegl_tile_backend_file_write_header (self);
      self->foffset       = 256;
      fsync (self->o);
      self->i = dup (self->o);

      /*self->i = G_INPUT_STREAM (g_file_read (self->file, NULL, NULL));*/
      self->next_pre_alloc = 256;  /* reserved space for header */
      self->total          = 256;  /* reserved space for header */
      g_assert (self->i != -1);
      g_assert (self->o != -1);
    }
}

static void
gegl_tile_backend_file_class_init (GeglTileBackendFileClass *klass)
{
  GObjectClass    *gobject_class     = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = gegl_tile_backend_file_get_property;
  gobject_class->set_property = gegl_tile_backend_file_set_property;
  gobject_class->constructor  = gegl_tile_backend_file_constructor;
  gobject_class->finalize     = gegl_tile_backend_file_finalize;


  GEGL_BUFFER_STRUCT_CHECK_PADDING;

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_backend_file_init (GeglTileBackendFile *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_backend_file_command;
  self->path           = NULL;
  self->file           = NULL;
  self->i              = -1;
  self->o              = -1;
  self->index          = NULL;
  self->free_list      = NULL;
  self->next_pre_alloc = 256;  /* reserved space for header */
  self->total          = 256;  /* reserved space for header */
}

gboolean
gegl_tile_backend_file_try_lock (GeglTileBackendFile *self)
{
  GeglBufferHeader new_header;
  new_header = gegl_buffer_read_header (self->i, NULL)->header;
  if (new_header.flags & GEGL_FLAG_LOCKED)
    {
      return FALSE;
    }
  self->header.flags += GEGL_FLAG_LOCKED;
  gegl_tile_backend_file_write_header (self);
  fsync (self->o);
  return TRUE;
}

gboolean
gegl_tile_backend_file_unlock (GeglTileBackendFile *self)
{
  if (!(self->header.flags & GEGL_FLAG_LOCKED))
    {
      g_warning ("tried to unlock unlocked buffer");
      return FALSE;
    }
  self->header.flags -= GEGL_FLAG_LOCKED;
  gegl_tile_backend_file_write_header (self);
  fsync (self->o);
  return TRUE;
}
