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
 *           2012 Ville Sokk <ville.sokk@gmail.com>
 */

/* GeglTileBackendFile stores tiles of a GeglBuffer on disk. There are
 * two versions of the class. This one memory maps the file and I/O is
 * performed using memcpy. There's a thread that performs writes like the
 * async backend since the underlying file has to be grown often which
 * has to be done with a system call which causes blocking. If the thread
 * queue is empty then regular memcpy is used instead of pushing to the
 * queue.
 */

#include "config.h"

#include <gio/gio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "gegl.h"
#include "gegl-buffer-backend.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-file.h"
#include "gegl-buffer-index.h"
#include "gegl-buffer-types.h"
#include "gegl-debug.h"
#include "gegl-config.h"


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

  /* current offset, used when writing the index */
  gint             offset;

  /* when writing buffer blocks the writer keeps one block unwritten
   * at all times to be able to keep track of the ->next offsets in
   * the blocks.
   */
  GeglFileBackendEntry *in_holding;

  /* loading buffer */
  GList           *tiles;

  /* GFile refering to our buffer */
  GFile           *file;

  GFileMonitor    *monitor;
  /* for writing */
  int              o;

  /* for reading */
  int              i;

  /* pointer to the memory mapped file */
  gchar           *map;

  /* size of the memory mapped area */
  guint            total_mapped;

  /* number of write operations in the queue for this file */
  gint             pending_ops;

  /* used for waiting on writes to the file to be finished */
  GCond           *cond;
};


static void     gegl_tile_backend_file_ensure_exist (GeglTileBackendFile  *self);
static gboolean gegl_tile_backend_file_write_block  (GeglTileBackendFile  *self,
                                                     GeglFileBackendEntry *block);
static void     gegl_tile_backend_file_dbg_alloc    (int                   size);
static void     gegl_tile_backend_file_dbg_dealloc  (int                   size);


G_DEFINE_TYPE (GeglTileBackendFile, gegl_tile_backend_file, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;

/* this debugging is across all buffers */
static gint allocs         = 0;
static gint file_size      = 0;
static gint peak_allocs    = 0;
static gint peak_file_size = 0;

static GQueue  queue      = G_QUEUE_INIT;
static GMutex *mutex      = NULL;
static GCond  *queue_cond = NULL;
static GCond  *max_cond   = NULL;
static GeglFileBackendThreadParams *in_progress;


static void
gegl_tile_backend_file_finish_writing (GeglTileBackendFile *self)
{
  g_mutex_lock (mutex);
  while (self->pending_ops != 0)
    g_cond_wait (self->cond, mutex);
  g_mutex_unlock (mutex);
}

static void
gegl_tile_backend_file_push_queue (GeglFileBackendThreadParams *params)
{
  guint length;

  g_mutex_lock (mutex);

  length = g_queue_get_length (&queue);

  if (length > gegl_config()->queue_limit)
    g_cond_wait (max_cond, mutex);

  params->file->pending_ops += 1;
  g_queue_push_tail (&queue, params);

  if (params->entry)
    {
      if (params->operation == OP_WRITE)
        params->entry->tile_link = g_queue_peek_tail_link (&queue);
      else /* OP_WRITE_BLOCK */
        params->entry->block_link = g_queue_peek_tail_link (&queue);
    }

  /* wake up the writer thread */
  g_cond_signal (queue_cond);

  g_mutex_unlock (mutex);
}

static void
gegl_tile_backend_file_write (GeglTileBackendFile     *self,
                              goffset                  offset,
                              gchar                   *source,
                              guint                    length,
                              GeglFileBackendEntry    *entry,
                              GeglFileBackendThreadOp  operation)
{
  if (g_queue_is_empty (&queue))
    {
      memcpy (self->map + offset, source, length);
    }
  else
    {
      GeglFileBackendThreadParams *params = NULL;
      guchar *new_source;

      if (entry && (entry->tile_link || entry->block_link))
        {
          gchar *msg;

          g_mutex_lock (mutex);

          if (operation == OP_WRITE && entry->tile_link)
            {
              params = entry->tile_link->data;
              msg = "entry";
            }
          else if (operation == OP_WRITE_BLOCK && entry->block_link)
            {
              params = entry->block_link->data;
              msg = "block";
            }

          if (params)
            {
              memcpy (params->source, source, length);
              params->offset = offset;
              g_mutex_unlock (mutex);

              GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "overwrote queue %s %i,%i,%i at %i", msg, entry->tile->x, entry->tile->y, entry->tile->z, (gint)entry->tile->offset);

              return;
            }

          g_mutex_unlock (mutex);
        }

      params = g_new0 (GeglFileBackendThreadParams, 1);
      new_source = g_malloc (length);

      memcpy (new_source, source, length);

      params->operation = operation;
      params->offset    = offset;
      params->file      = self;
      params->length    = length;
      params->source    = new_source;
      params->entry     = entry;

      gegl_tile_backend_file_push_queue (params);
    }
}

static inline void
gegl_tile_backend_file_map (GeglTileBackendFile *self)
{
  self->map = mmap (NULL, self->total_mapped, PROT_READ|PROT_WRITE, MAP_SHARED, self->o, 0);
  if (self->map == MAP_FAILED)
    {
      g_error ("Unable to memory map file %s: %s", self->path, g_strerror (errno));
      return;
    }
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "mapped %u bytes of file %s", self->total_mapped, self->path);
}

static inline void
gegl_tile_backend_file_unmap (GeglTileBackendFile *self)
{
  if ((munmap (self->map, self->total_mapped)) < 0)
    {
      g_warning ("Unable to unmap file %s: %s", self->path, g_strerror (errno));
      return;
    }
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "unmapped file %s", self->path);
}

static gpointer
gegl_tile_backend_file_writer_thread (gpointer ignored)
{
  while (TRUE)
    {
      GeglFileBackendThreadParams *params;

      g_mutex_lock (mutex);

      while (g_queue_is_empty (&queue))
        g_cond_wait (queue_cond, mutex);

      params = (GeglFileBackendThreadParams *)g_queue_pop_head (&queue);
      if (params->entry)
        {
          in_progress = params;
          if (params->operation == OP_WRITE)
            params->entry->tile_link = NULL;
          else /* OP_WRITE_BLOCK */
            params->entry->block_link = NULL;
        }
      g_mutex_unlock (mutex);

      switch (params->operation)
        {
        case OP_WRITE:
          memcpy (params->file->map + params->offset,
                  params->source, params->length);
          break;
        case OP_WRITE_BLOCK:
          memcpy (params->file->map + params->offset,
                  params->source, params->length);
          break;
        case OP_TRUNCATE:
          {
            GeglTileBackendFile *file = params->file;
            ftruncate (file->o, params->length);
            if (file->total > file->total_mapped)
              {
                g_mutex_lock (mutex);

                gegl_tile_backend_file_unmap (file);
                file->total_mapped = file->total * 5;
                gegl_tile_backend_file_map (file);

                g_mutex_unlock (mutex);
              }
          }
          break;
        default: /* OP_SYNC is not necessary for memory mapped files */
          break;
        }

      g_mutex_lock (mutex);
      in_progress = NULL;

      params->file->pending_ops -= 1;
      if (params->file->pending_ops == 0)
        g_cond_signal (params->file->cond);

      if (g_queue_get_length (&queue) < gegl_config ()->queue_limit)
        g_cond_signal (max_cond);

      if (params->source)
        g_free (params->source);

      g_free (params);

      g_mutex_unlock (mutex);
    }

  return NULL;
}

static inline void
gegl_tile_backend_file_file_entry_read (GeglTileBackendFile  *self,
                                        GeglFileBackendEntry *entry,
                                        guchar               *dest)
{
  gint     tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  goffset  offset    = entry->tile->offset;

  gegl_tile_backend_file_ensure_exist (self);

  if (entry->tile_link || in_progress)
    {
      GeglFileBackendThreadParams *queued_op = NULL;
      g_mutex_lock (mutex);

      if (entry->tile_link)
        queued_op = entry->tile_link->data;
      else if (in_progress && in_progress->entry == entry &&
               in_progress->operation == OP_WRITE)
        queued_op = in_progress;

      if (queued_op)
        {
          memcpy (dest, queued_op->source, tile_size);
          g_mutex_unlock (mutex);

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i from queue", entry->tile->x, entry->tile->y, entry->tile->z);

          return;
        }

      g_mutex_unlock (mutex);
    }

  memcpy (dest, self->map + offset, tile_size);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i at %i", entry->tile->x, entry->tile->y, entry->tile->z, (gint)offset);
}

static GeglFileBackendEntry *
gegl_tile_backend_file_file_entry_create (gint x,
                                          gint y,
                                          gint z)
{
  GeglFileBackendEntry *entry = g_new0 (GeglFileBackendEntry, 1);

  entry->tile       = gegl_tile_entry_new (x, y, z);
  entry->tile_link  = NULL;
  entry->block_link = NULL;

  return entry;
}

static inline GeglFileBackendEntry *
gegl_tile_backend_file_file_entry_new (GeglTileBackendFile *self)
{
  GeglFileBackendEntry *entry = gegl_tile_backend_file_file_entry_create (0,0,0);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Creating new entry");

  gegl_tile_backend_file_ensure_exist (self);

  if (self->free_list)
    {
      /* XXX: losing precision ?
       * the free list seems to operate with fixed size datums and
       * only keep track of offsets.
       */
      gint offset = GPOINTER_TO_INT (self->free_list->data);
      entry->tile->offset = offset;
      self->free_list = g_slist_remove (self->free_list, self->free_list->data);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i from free list", ((gint)entry->tile->offset));
    }
  else
    {
      gint tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));

      entry->tile->offset = self->next_pre_alloc;
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "  set offset %i (next allocation)", (gint)entry->tile->offset);
      self->next_pre_alloc += tile_size;

      if (self->next_pre_alloc >= self->total) /* automatic growing ensuring that
                                                  we have room for next allocation..
                                                */
        {
          GeglFileBackendThreadParams *params = g_new0 (GeglFileBackendThreadParams, 1);

          self->total = self->total + 32 * tile_size;

          params->operation = OP_TRUNCATE;
          params->file      = self;
          params->length    = self->total;

          gegl_tile_backend_file_push_queue (params);

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed truncate to %i bytes", (gint)self->total);
        }
    }
  gegl_tile_backend_file_dbg_alloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
  return entry;
}

static inline void
gegl_tile_backend_file_file_entry_destroy (GeglFileBackendEntry *entry,
                                           GeglTileBackendFile  *self)
{
  /* XXX: EEEk, throwing away bits */
  guint offset = entry->tile->offset;

  if (entry->tile_link || entry->block_link)
    {
      gint   i;
      GList *link;

      g_mutex_lock (mutex);

      for (i = 0, link = entry->tile_link;
           i < 2;
           i++, link = entry->block_link)
        {
          if (link)
            {
              GeglFileBackendThreadParams *queued_op = link->data;
              queued_op->file->pending_ops -= 1;
              g_queue_delete_link (&queue, link);
              g_free (queued_op->source);
              g_free (queued_op);
            }
        }

      g_mutex_unlock (mutex);
    }

  self->free_list = g_slist_prepend (self->free_list,
                                     GUINT_TO_POINTER (offset));
  g_hash_table_remove (self->index, entry);

  gegl_tile_backend_file_dbg_dealloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));

  g_free (entry->tile);
  g_free (entry);
}

static gboolean
gegl_tile_backend_file_write_header (GeglTileBackendFile *self)
{
  gegl_tile_backend_file_ensure_exist (self);

  gegl_tile_backend_file_write (self, 0, (gchar*)&(self->header), 256, NULL, OP_WRITE);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Wrote header, next=%i", (gint)self->header.next);
  return TRUE;
}

static gboolean
gegl_tile_backend_file_write_block (GeglTileBackendFile  *self,
                                    GeglFileBackendEntry *item)
{
  gegl_tile_backend_file_ensure_exist (self);
  if (self->in_holding)
    {
      GeglBufferBlock *block           = &(self->in_holding->tile->block);
      guint64          next_allocation = self->offset + block->length;
      gint             length          = block->length;

      /* update the next offset pointer in the previous block */
      if (item == NULL)
          /* the previous block was the last block */
          block->next = 0;
      else
          block->next = next_allocation;

      /* XXX: should promiscuosuly try to compress here as well,. if revisions
              are not matching..
       */

      gegl_tile_backend_file_write (self, self->offset, (gchar*)block, length,
                                    self->in_holding, OP_WRITE_BLOCK);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Wrote block: length:%i flags:%i next:%i at offset %i",
                 length,
                 block->flags,
                 (gint)block->next,
                 self->offset);

      self->offset = next_allocation;
    }
  else
    {
      /* we're setting up for the first write */
      self->offset = self->next_pre_alloc; /* start writing header at end
                                            * of file, worry about writing
                                            * header inside free list later
                                            */
    }
  self->in_holding = item;

  return TRUE;
}

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

static inline GeglFileBackendEntry *
gegl_tile_backend_file_lookup_entry (GeglTileBackendFile *self,
                                     gint                 x,
                                     gint                 y,
                                     gint                 z)
{
  GeglFileBackendEntry *ret = NULL;
  GeglFileBackendEntry *key = gegl_tile_backend_file_file_entry_create (x,y,z);
  ret = g_hash_table_lookup (self->index, key);
  g_free (key->tile);
  g_free (key);
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
  GeglTileBackend      *backend;
  GeglTileBackendFile  *tile_backend_file;
  GeglFileBackendEntry *entry;
  GeglTile             *tile = NULL;
  gint                  tile_size;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (!entry)
    return NULL;

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  tile      = gegl_tile_new (tile_size);
  gegl_tile_set_rev (tile, entry->tile->rev);
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
  GeglTileBackend      *backend;
  GeglTileBackendFile  *tile_backend_file;
  GeglFileBackendEntry *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (entry == NULL)
    {
      entry          = gegl_tile_backend_file_file_entry_new (tile_backend_file);
      entry->tile->x = x;
      entry->tile->y = y;
      entry->tile->z = z;
      g_hash_table_insert (tile_backend_file->index, entry, entry);
    }
  entry->tile->rev = gegl_tile_get_rev (tile);

  gegl_tile_backend_file_write (tile_backend_file, entry->tile->offset,
                                (gchar*)gegl_tile_get_data (tile),
                                gegl_tile_backend_get_tile_size (backend),
                                entry, OP_WRITE);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "wrote entry %i, %i, %i at %i", entry->tile->x, entry->tile->y, entry->tile->z, (gint)entry->tile->offset);

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
  GeglTileBackend      *backend;
  GeglTileBackendFile  *tile_backend_file;
  GeglFileBackendEntry *entry;

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
  GeglTileBackend      *backend;
  GeglTileBackendFile  *tile_backend_file;
  GeglFileBackendEntry *entry;

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
          GeglFileBackendEntry *item = iter->data;

          gegl_tile_backend_file_write_block (self, item);
        }
      gegl_tile_backend_file_write_block (self, NULL); /* terminate the index */
      g_list_free (tiles);
    }

  gegl_tile_backend_file_write_header (self);

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
      gegl_tile_backend_file_finish_writing (self);
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "finalizing buffer %s", self->path);

      if (self->o != -1)
        {
          close (self->o);
          self->o = -1;
        }
      if (self->map)
        gegl_tile_backend_file_unmap (self);
    }

  if (self->path)
    {
      gegl_tile_backend_unlink_swap (self->path);
      g_free (self->path);
    }

  if (self->monitor)
    g_object_unref (self->monitor);

  if (self->file)
    g_object_unref (self->file);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static guint
gegl_tile_backend_file_hashfunc (gconstpointer key)
{
  const GeglBufferTile *e    = ((GeglFileBackendEntry *)key)->tile;
  guint                 hash;
  gint                  i;
  gint                  srcA = e->x;
  gint                  srcB = e->y;
  gint                  srcC = e->z;

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
  const GeglBufferTile *ea = ((GeglFileBackendEntry*)a)->tile;
  const GeglBufferTile *eb = ((GeglFileBackendEntry*)b)->tile;

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
  GeglBufferHeader  new_header;
  GList            *iter;
  GeglTileBackend  *backend;
  goffset           offset = 0;
  goffset           max    = 0;
  gint              tile_size;

  /* compute total from and next pre alloc by monitoring tiles as they
   * are added here
   */
  /* reload header */
  new_header = gegl_buffer_read_header (0, &offset, self->map)->header;

  while (new_header.flags & GEGL_FLAG_LOCKED)
    {
      g_usleep (50000);
      new_header = gegl_buffer_read_header (0, &offset, self->map)->header;
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

  tile_size     = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  offset        = self->header.next;
  self->tiles   = gegl_buffer_read_index (0, &offset, self->map);
  backend       = GEGL_TILE_BACKEND (self);

  for (iter = self->tiles; iter; iter=iter->next)
    {
      GeglBufferItem       *item     = iter->data;
      GeglFileBackendEntry *new      = gegl_tile_backend_file_file_entry_create (0, 0, 0);
      GeglFileBackendEntry *existing =
        gegl_tile_backend_file_lookup_entry (self, item->tile.x, item->tile.y, item->tile.z);

      if (item->tile.offset > max)
        max = item->tile.offset + tile_size;

      if (existing)
        {
          if (existing->tile->rev == item->tile.rev)
            {
              g_assert (existing->tile->offset == item->tile.offset);
              *existing->tile = item->tile;
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
                                        existing->tile->x,
                                        existing->tile->y,
                                        existing->tile->z);

              if (existing->tile->z == 0)
                {
                  rect.width = self->header.tile_width;
                  rect.height = self->header.tile_height;
                  rect.x = existing->tile->x * self->header.tile_width;
                  rect.y = existing->tile->y * self->header.tile_height;
                }
              g_free (existing);

              g_signal_emit_by_name (storage, "changed", &rect, NULL);
            }
        }
      new->tile = iter->data;
      g_hash_table_insert (self->index, new, new);
    }
  g_list_free (self->tiles);
  g_slist_free (self->free_list);
  self->free_list      = NULL;
  self->next_pre_alloc = max; /* if bigger than own? */
  self->tiles          = NULL;
  self->total          = max;
}

static void
gegl_tile_backend_file_file_changed (GFileMonitor        *monitor,
                                     GFile               *file,
                                     GFile               *other_file,
                                     GFileMonitorEvent    event_type,
                                     GeglTileBackendFile *self)
{
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED /*G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT*/ )
      gegl_tile_backend_file_load_index (self, TRUE);
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

  self->file        = g_file_new_for_commandline_arg (self->path);
  self->o           = -1;
  self->index       = g_hash_table_new (gegl_tile_backend_file_hashfunc,
                                        gegl_tile_backend_file_equalfunc);
  self->pending_ops = 0;
  self->cond        = g_cond_new ();

  /* If the file already exists open it, assuming it is a GeglBuffer. */
  if (g_access (self->path, F_OK) != -1)
    {
      GStatBuf  stats;

      /* Install a monitor for changes to the file in case other applications
       * might be writing to the buffer
       */
      self->monitor = g_file_monitor_file (self->file, G_FILE_MONITOR_NONE,
                                           NULL, NULL);
      g_signal_connect (self->monitor, "changed",
                        G_CALLBACK (gegl_tile_backend_file_file_changed),
                        self);

      self->o = g_open (self->path, O_RDWR|O_CREAT, 0770);
      if (self->o == -1)
        {
          /* Try again but this time with only read access. This is
           * a quick-fix for make distcheck, where img_cmp fails
           * when it opens a GeglBuffer file in the source tree
           * (which is read-only).
           */
          self->o = g_open (self->path, O_RDONLY, 0770);

          if (self->o == -1)
            g_warning ("%s: Could not open '%s': %s", G_STRFUNC, self->path, g_strerror (errno));
        }

      g_stat (self->path, &stats);
      self->total_mapped = stats.st_size;
      gegl_tile_backend_file_map (self);

      self->header = gegl_buffer_read_header (0, NULL, self->map)->header;
      self->header.rev = self->header.rev -1;

      /* we are overriding all of the work of the actual constructor here,
       * a really evil hack :d
       */
      backend->priv->tile_width  = self->header.tile_width;
      backend->priv->tile_height = self->header.tile_height;
      backend->priv->format      = babl_format (self->header.description);
      backend->priv->px_size     = babl_format_get_bytes_per_pixel (backend->priv->format);
      backend->priv->tile_size   = backend->priv->tile_width *
                                    backend->priv->tile_height *
                                    backend->priv->px_size;

      /* insert each of the entries into the hash table */
      gegl_tile_backend_file_load_index (self, TRUE);
      self->exist = TRUE;
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

      self->o = g_open (self->path, O_RDWR|O_CREAT, 0770);
      if (self->o == -1)
        g_warning ("%s: Could not open '%s': %s", G_STRFUNC, self->path, g_strerror (errno));

      self->next_pre_alloc = 256;  /* reserved space for header */
      self->total          = 256;  /* reserved space for header */

      ftruncate (self->o, 256);
      self->total_mapped = 2000000000;
      gegl_tile_backend_file_map (self);

      gegl_buffer_header_init (&self->header,
                               backend->priv->tile_width,
                               backend->priv->tile_height,
                               backend->priv->px_size,
                               backend->priv->format
                               );
      gegl_tile_backend_file_write_header (self);

      g_assert (self->o != -1);
    }
}

static void
gegl_tile_backend_file_class_init (GeglTileBackendFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = gegl_tile_backend_file_get_property;
  gobject_class->set_property = gegl_tile_backend_file_set_property;
  gobject_class->constructor  = gegl_tile_backend_file_constructor;
  gobject_class->finalize     = gegl_tile_backend_file_finalize;

  queue_cond = g_cond_new ();
  max_cond   = g_cond_new ();
  mutex      = g_mutex_new ();
  g_thread_create_full (gegl_tile_backend_file_writer_thread,
                        NULL, 0, TRUE, TRUE, G_THREAD_PRIORITY_NORMAL, NULL);

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
  self->path                       = NULL;
  self->file                       = NULL;
  self->o                          = -1;
  self->index                      = NULL;
  self->free_list                  = NULL;
  self->next_pre_alloc             = 256; /* reserved space for header */
  self->total                      = 256; /* reserved space for header */
  self->map                        = NULL;
  self->pending_ops                = 0;
}

gboolean
gegl_tile_backend_file_try_lock (GeglTileBackendFile *self)
{
  GeglBufferHeader new_header;

  new_header = gegl_buffer_read_header (0, NULL, self->map)->header;
  if (new_header.flags & GEGL_FLAG_LOCKED)
    {
      return FALSE;
    }
  self->header.flags += GEGL_FLAG_LOCKED;
  gegl_tile_backend_file_write_header (self);
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

  /* wait until all writes to this file are finished before handing it over
     to another process */
  gegl_tile_backend_file_finish_writing (self);

  return TRUE;
}
