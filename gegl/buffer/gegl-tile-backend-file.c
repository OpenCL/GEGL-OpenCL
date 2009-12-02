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

#if HAVE_GIO
#include <gio/gio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-file.h"
#include "gegl-buffer-index.h"
#include "gegl-debug.h"
#include "gegl-types-internal.h"


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

  /* current offset, used when writing the index */
  gint             offset;

  /* when writing buffer blocks the writer keeps one block unwritten
   * at all times to be able to keep track of the ->next offsets in
   * the blocks.
   */
  GeglBufferBlock *in_holding; 

  /* loading buffer */
  GList           *tiles;

#if HAVE_GIO
  /* GFile refering to our buffer */
  GFile           *file;

  /* for reading/writing */
  GIOStream       *io;

  /* for reading */
  GInputStream    *i;

  /* for writing */
  GOutputStream   *o;


  /* Before using mmap we'll use GIO's infrastructure for monitoring
   * the file for changes, this should also be more portable. This is
   * used for for cooperated sharing of a buffer file
   */
  GFileMonitor    *monitor;
#else
  /* for writing */
  int              o;

  /* for reading */
  int              i;
#endif
};


static void     gegl_tile_backend_file_ensure_exist (GeglTileBackendFile *self);
static gboolean gegl_tile_backend_file_write_block  (GeglTileBackendFile *self,
                                                     GeglBufferBlock     *block);
static void     gegl_tile_backend_file_dbg_alloc    (int                  size);
static void     gegl_tile_backend_file_dbg_dealloc  (int                  size);


static void inline
gegl_tile_backend_file_file_entry_read (GeglTileBackendFile *self,
                                        GeglBufferTile      *entry,
                                        guchar              *dest)
{
  gint     to_be_read;
  gboolean success;
  gint     tile_size = GEGL_TILE_BACKEND (self)->tile_size;
  goffset  offset = entry->offset;

  gegl_tile_backend_file_ensure_exist (self);

#if HAVE_GIO
  success = g_seekable_seek (G_SEEKABLE (self->i),
                             offset, G_SEEK_SET,
                             NULL, NULL);
#else
  success = (lseek (self->i, offset, SEEK_SET) >= 0);
#endif
  if (success == FALSE)
    {
      g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
      return;
    }
  to_be_read = tile_size;

  while (to_be_read > 0)
    {
      gint byte_read;

#if HAVE_GIO
      byte_read = g_input_stream_read (G_INPUT_STREAM (self->i),
                                       dest + tile_size - to_be_read, to_be_read,
                                       NULL, NULL);
#else
      byte_read = read (self->i, dest + tile_size - to_be_read, to_be_read);
#endif
      if (byte_read <= 0)
        {
          g_message ("unable to read tile data from self: "
                     "%s (%d/%d bytes read)",
                     g_strerror (errno), byte_read, to_be_read);
          return;
        }
      to_be_read -= byte_read;
    }


  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i at %i", entry->x, entry->y, entry->z, (gint)offset);
}

static void inline
gegl_tile_backend_file_file_entry_write (GeglTileBackendFile *self,
                                         GeglBufferTile      *entry,
                                         guchar              *source)
{
  gint     to_be_written;
  gboolean success;
  gint     tile_size = GEGL_TILE_BACKEND (self)->tile_size;
  goffset  offset = entry->offset;

  gegl_tile_backend_file_ensure_exist (self);

#if HAVE_GIO
  success = g_seekable_seek (G_SEEKABLE (self->o),
                             offset, G_SEEK_SET,
                             NULL, NULL);
#else
  success = (lseek (self->o, offset, SEEK_SET) >= 0);
#endif
  if (success == FALSE)
    {
      g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
      return;
    }
  to_be_written = tile_size;

  while (to_be_written > 0)
    {
      gint wrote;
#if HAVE_GIO
      wrote = g_output_stream_write (self->o,
                                     source + tile_size - to_be_written,
                                     to_be_written, NULL, NULL);
#else
      wrote = write (self->o,
                     source + tile_size - to_be_written,
                     to_be_written);
#endif
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          return;
        }
      to_be_written -= wrote;
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
      /* XXX: losing precision ? */
      gint offset = GPOINTER_TO_INT (self->free_list->data);
      entry->offset = offset;
      self->free_list = g_slist_remove (self->free_list, self->free_list->data);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i from free list", ((gint)entry->offset));
    }
  else
    {
      gint tile_size = GEGL_TILE_BACKEND (self)->tile_size;

      entry->offset = self->next_pre_alloc;
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i (next allocation)", (gint)entry->offset);
      self->next_pre_alloc += tile_size;

      if (self->next_pre_alloc >= self->total)
        {
          self->total = self->total + 32 * tile_size;

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "growing file to %i bytes", (gint)self->total);

#if HAVE_GIO
          g_assert (g_seekable_truncate (G_SEEKABLE (self->o),
                                         self->total, NULL,NULL));
#else
          g_assert (ftruncate (self->o, self->total) == 0);
#endif
        }
    }
  gegl_tile_backend_file_dbg_alloc (GEGL_TILE_BACKEND (self)->tile_size);
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

  gegl_tile_backend_file_dbg_dealloc (GEGL_TILE_BACKEND (self)->tile_size);
  g_free (entry);
}

static gboolean
gegl_tile_backend_file_write_header (GeglTileBackendFile *self)
{
  gboolean success;

  gegl_tile_backend_file_ensure_exist (self);

#if HAVE_GIO
  success = g_seekable_seek (G_SEEKABLE (self->o), 0, G_SEEK_SET,
                             NULL, NULL);
#else
  success = (lseek (self->o, 0, SEEK_SET) != -1);
#endif
  if (success == FALSE)
    {
      g_warning ("unable to seek in buffer");
      return FALSE;
    }
#if HAVE_GIO
  g_output_stream_write (self->o, &(self->header), 256, NULL, NULL);
#else
  write (self->o, &(self->header), 256);
#endif
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
      self->in_holding->next = next_allocation;

      if (block == NULL) /* the previous block was the last block */
        {
          self->in_holding->next = 0;
        }

#if HAVE_GIO
      if(!g_seekable_seek (G_SEEKABLE (self->o),
                           self->offset, G_SEEK_SET,
                           NULL, NULL))
#else
      if(lseek (self->o, self->offset, G_SEEK_SET) == -1)
#endif
        goto fail;

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Wrote block: length:%i flags:%i next:%i at offset %i",
                 self->in_holding->length,
                 self->in_holding->flags,
                 (gint)self->in_holding->next,
                 (gint)self->offset);
#if HAVE_GIO
      self->offset += g_output_stream_write (self->o, self->in_holding,
                                             self->in_holding->length,
                                             NULL, NULL);
#else
      {
        ssize_t written = write (self->o, self->in_holding,
                                 self->in_holding->length);
        if(written != -1)
          self->offset += written;
      }
#endif

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
#if HAVE_GIO
      if(!g_seekable_seek (G_SEEKABLE (self->o),
                           (goffset) self->offset, G_SEEK_SET,
                           NULL, NULL))
#else
      if(lseek (self->o, self->offset, G_SEEK_SET) == -1)
#endif
        goto fail;
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

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (!entry)
    return NULL;

  tile             = gegl_tile_new (backend->tile_size);
  tile->rev        = entry->rev;
  tile->stored_rev = entry->rev;

  gegl_tile_backend_file_file_entry_read (tile_backend_file, entry, tile->data);
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
  entry->rev = tile->rev;

  gegl_tile_backend_file_file_entry_write (tile_backend_file, entry, tile->data);
  tile->stored_rev = tile->rev;
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
#if HAVE_GIO
  g_output_stream_flush (self->o, NULL, NULL);
#else
  fsync (self->o);
#endif

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

#if HAVE_GIO
      if (self->io)
        {
          g_io_stream_close (self->io, NULL, NULL);
          g_object_unref (self->io);
          self->io = NULL;
        }

      if (self->file)
        g_file_delete  (self->file, NULL, NULL);
#else
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
#endif
    }

  if (self->path)
    g_free (self->path);

#if HAVE_GIO
  if (self->monitor)
    g_object_unref (self->monitor);

  if (self->file)
    g_object_unref (self->file);
#endif

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

  /* compute total from and next pre alloc by monitoring tiles as they
   * are added here
   */
  /* reload header */
  new_header = gegl_buffer_read_header (self->i, &offset)->header;

  while (new_header.flags & GEGL_FLAG_LOCKED)
    {
      g_usleep (50000);
      new_header = gegl_buffer_read_header (self->i, &offset)->header;
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


  offset      = self->header.next;
  self->tiles = gegl_buffer_read_index (self->i, &offset);
  backend     = GEGL_TILE_BACKEND (self);

  for (iter = self->tiles; iter; iter=iter->next)
    {
      GeglBufferItem *item = iter->data;

      GeglBufferItem *existing = g_hash_table_lookup (self->index, item);

      if (item->tile.offset > max)
        max = item->tile.offset + backend->tile_size;

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
              GeglRectangle rect;
              g_hash_table_remove (self->index, existing);
              gegl_tile_source_refetch (GEGL_TILE_SOURCE (backend->storage),
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
              g_signal_emit_by_name (backend->storage, "changed", &rect, NULL);
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

#if HAVE_GIO
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
    }
}
#endif

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
#if HAVE_GIO
  self->file = g_file_new_for_commandline_arg (self->path);
#else
  self->i = self->o = -1;
#endif
  self->index = g_hash_table_new (gegl_tile_backend_file_hashfunc, gegl_tile_backend_file_equalfunc);


  /* If the file already exists open it, assuming it is a GeglBuffer. */
#if HAVE_GIO
  if (g_file_query_exists (self->file, NULL))
#else
  if (access (self->path, F_OK) != -1)
#endif
    {
      goffset offset = 0;

#if HAVE_GIO
      /* Install a monitor for changes to the file in case other applications
       * might be writing to the buffer
       */
      self->monitor = g_file_monitor_file (self->file, G_FILE_MONITOR_NONE,
                                           NULL, NULL);
      g_signal_connect (self->monitor, "changed",
                        G_CALLBACK (gegl_tile_backend_file_file_changed),
                        self);

      {
        GError *error = NULL;
        self->io = G_IO_STREAM (g_file_open_readwrite (self->file, NULL, &error));
        if (error)
          {
            g_warning ("%s: %s", G_STRLOC, error->message);
            g_error_free (error);
            error = NULL;
          }
        self->o = g_io_stream_get_output_stream (self->io);
        self->i = g_io_stream_get_input_stream (self->io);
      }

#else
      self->o = open (self->path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
      self->i = dup (self->o);
#endif
      /*self->i = G_INPUT_STREAM (g_file_read (self->file, NULL, NULL));*/
      self->header = gegl_buffer_read_header (self->i, &offset)->header;
      self->header.rev = self->header.rev -1;

      /* we are overriding all of the work of the actual constructor here */
      backend->tile_width = self->header.tile_width;
      backend->tile_height = self->header.tile_height;
      backend->format = babl_format (self->header.description);
      backend->px_size = babl_format_get_bytes_per_pixel (backend->format);
      backend->tile_size = backend->tile_width * backend->tile_height * backend->px_size;

      /* insert each of the entries into the hash table */
      gegl_tile_backend_file_load_index (self, TRUE);
      self->exist = TRUE;
#if HAVE_GIO
      g_assert (self->i);
      g_assert (self->o);
#else
      g_assert (self->i != -1);
      g_assert (self->o != -1);
#endif

      /* to autoflush gegl_buffer_set */
      backend->shared = TRUE;
    }
  else
    {
      self->exist = FALSE; /* this is also the default, the file will be created on demand */
    }

#if HAVE_GIO
  g_assert (self->file);
#endif

  backend->header = &self->header;

  return object;
}

static void
gegl_tile_backend_file_ensure_exist (GeglTileBackendFile *self)
{
  if (!self->exist)
    {
      GeglTileBackend *backend;
#if HAVE_GIO
      GError *error = NULL;

      if (self->io)
        {
          g_print ("we already existed\n");
          return;
        }
#endif
      self->exist = TRUE;

      backend = GEGL_TILE_BACKEND (self);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "creating swapfile  %s", self->path);

#if HAVE_GIO
      self->o = G_OUTPUT_STREAM (g_file_replace (self->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL));
      g_output_stream_flush (self->o, NULL, NULL);
      g_output_stream_close (self->o, NULL, NULL);

      self->io = G_IO_STREAM (g_file_open_readwrite (self->file, NULL, &error));
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error->message);
          g_error_free (error);
          error = NULL;
        }
      self->o = g_io_stream_get_output_stream (self->io);
      self->i = g_io_stream_get_input_stream (self->io);

#else
      self->o = open (self->path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif

      self->next_pre_alloc = 256;  /* reserved space for header */
      self->total          = 256;  /* reserved space for header */
#if HAVE_GIO
      g_assert(g_seekable_seek (G_SEEKABLE (self->o), 256, G_SEEK_SET, NULL, NULL));
#endif
      gegl_buffer_header_init (&self->header,
                               backend->tile_width,
                               backend->tile_height,
                               backend->px_size,
                               backend->format
                               );
      gegl_tile_backend_file_write_header (self);
#if HAVE_GIO
      g_output_stream_flush (self->o, NULL, NULL);
#else
      fsync (self->o);
      self->i = dup (self->o);
#endif

      /*self->i = G_INPUT_STREAM (g_file_read (self->file, NULL, NULL));*/
      self->next_pre_alloc = 256;  /* reserved space for header */
      self->total          = 256;  /* reserved space for header */
#if HAVE_GIO
      g_assert (self->io);
      g_assert (self->i);
      g_assert (self->o);
#else
      g_assert (self->i != -1);
      g_assert (self->o != -1);
#endif
    }
}

static void
gegl_tile_backend_file_class_init (GeglTileBackendFileClass *klass)
{
  GObjectClass    *gobject_class     = G_OBJECT_CLASS (klass);
  GeglTileSourceClass *gegl_tile_source_class = GEGL_TILE_SOURCE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = gegl_tile_backend_file_get_property;
  gobject_class->set_property = gegl_tile_backend_file_set_property;
  gobject_class->constructor  = gegl_tile_backend_file_constructor;
  gobject_class->finalize     = gegl_tile_backend_file_finalize;

  gegl_tile_source_class->command = gegl_tile_backend_file_command;

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
  self->path           = NULL;
#if HAVE_GIO
  self->file           = NULL;
  self->io             = NULL;
  self->i              = NULL;
  self->o              = NULL;
#else
  self->i              = -1;
  self->o              = -1;
#endif
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
#if HAVE_GIO
  g_output_stream_flush (self->o, NULL, NULL);
#else
  fsync (self->o);
#endif
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
#if HAVE_GIO
  g_output_stream_flush (self->o, NULL, NULL);
#else
  fsync (self->o);
#endif
  return TRUE;
}
