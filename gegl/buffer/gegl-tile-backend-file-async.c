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
 * two versions of the class. This one uses regular I/O calls in a
 * separate thread (shared between instances of the class) that performs
 * all file operations except reading and opening. Communication between
 * the main gegl thread and the writer thread is performed using a
 * queue. The writer thread sleeps if the queue is empty. If an entry is
 * read and the tile is in the queue then its data is copied from the
 * queue instead of read from disk. There are two locks, queue_mutex and
 * write_mutex. The first one is used to append to the queue or read from
 * it, the second one to completely stop the writer thread from working
 * (to remove/change queue entries).
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

  /* cached offsets of the file handles to avoid lseek syscall if possible */
  gint             in_offset;
  gint             out_offset;

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

  /* number of write operations in the queue for this file */
  gint             pending_ops;

  /* used for waiting on writes to the file to be finished */
  GCond            cond;

  /* for writing */
  int              o;

  /* for reading */
  int              i;
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
static GMutex  mutex      = { 0, };
static GCond   queue_cond = { 0, };
static GCond   max_cond   = { 0, };
static gint    queue_size = 0;
static GeglFileBackendThreadParams *in_progress;


static void
gegl_tile_backend_file_finish_writing (GeglTileBackendFile *self)
{
  g_mutex_lock (&mutex);
  while (self->pending_ops != 0)
    g_cond_wait (&self->cond, &mutex);
  g_mutex_unlock (&mutex);
}

static void
gegl_tile_backend_file_push_queue (GeglFileBackendThreadParams *params)
{
  g_mutex_lock (&mutex);

  /* block if the queue has gotten too big */
  while (queue_size > gegl_config ()->queue_size)
    g_cond_wait (&max_cond, &mutex);

  params->file->pending_ops += 1;
  g_queue_push_tail (&queue, params);

  if (params->entry)
    {
      if (params->operation == OP_WRITE)
        {
          params->entry->tile_link = g_queue_peek_tail_link (&queue);
          queue_size += params->length + sizeof (GList) +
            sizeof (GeglFileBackendThreadParams);
        }
      else /* OP_WRITE_BLOCK */
        params->entry->block_link = g_queue_peek_tail_link (&queue);
    }

  /* wake up the writer thread */
  g_cond_signal (&queue_cond);

  g_mutex_unlock (&mutex);
}

static inline void
gegl_tile_backend_file_write (GeglFileBackendThreadParams *params)
{
  gint    to_be_written = params->length;
  gint    fd            = params->file->o;
  goffset offset        = params->offset;

  if (params->file->out_offset != params->offset)
    {
      if (lseek (fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      params->file->out_offset = params->offset;
    }

  while (to_be_written > 0)
    {
      gint wrote;
      wrote = write (fd,
                     params->source + params->length - to_be_written,
                     to_be_written);
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          break;
        }

      to_be_written            -= wrote;
      params->file->out_offset += wrote;;
    }

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "writer thread wrote at %i", (gint)offset);
}

static gpointer
gegl_tile_backend_file_writer_thread (gpointer ignored)
{
  while (TRUE)
    {
      GeglFileBackendThreadParams *params;

      g_mutex_lock (&mutex);

      while (g_queue_is_empty (&queue))
        g_cond_wait (&queue_cond, &mutex);

      params = (GeglFileBackendThreadParams *)g_queue_pop_head (&queue);
      if (params->entry)
        {
          in_progress = params;
          if (params->operation == OP_WRITE)
            params->entry->tile_link = NULL;
          else /* OP_WRITE_BLOCK */
            params->entry->block_link = NULL;
        }
      g_mutex_unlock (&mutex);

      switch (params->operation)
        {
        case OP_WRITE:
          gegl_tile_backend_file_write (params);
          break;
        case OP_WRITE_BLOCK:
          gegl_tile_backend_file_write (params);
          break;
        case OP_TRUNCATE:
          if (ftruncate (params->file->o, params->length) != 0)
            g_warning ("failed to resize file: %s", g_strerror (errno));
          break;
        case OP_SYNC:
          fsync (params->file->o);
          break;
        }

      g_mutex_lock (&mutex);
      in_progress = NULL;

      /* the file maybe waiting for its file operations to finish */
      params->file->pending_ops -= 1;
      if (params->file->pending_ops == 0)
        g_cond_signal (&params->file->cond);

      if (params->operation == OP_WRITE)
        {
          queue_size -= params->length + sizeof (GList) +
            sizeof (GeglFileBackendThreadParams);
          g_free (params->source);

          /* unblock the main thread if the queue had gotten too big */
          if (queue_size < gegl_config ()->queue_size)
            g_cond_signal (&max_cond);
        }

      g_free (params);

      g_mutex_unlock (&mutex);
    }

  return NULL;
}

static void
gegl_tile_backend_file_entry_read (GeglTileBackendFile  *self,
                                   GeglFileBackendEntry *entry,
                                   guchar               *dest)
{
  gint    tile_size  = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  gint    to_be_read = tile_size;
  goffset offset     = entry->tile->offset;

  gegl_tile_backend_file_ensure_exist (self);

  if (entry->tile_link || in_progress)
    {
      GeglFileBackendThreadParams *queued_op = NULL;
      g_mutex_lock (&mutex);

      if (entry->tile_link)
        queued_op = entry->tile_link->data;
      else if (in_progress && in_progress->entry == entry &&
               in_progress->operation == OP_WRITE)
        queued_op = in_progress;

      if (queued_op)
        {
          memcpy (dest, queued_op->source, to_be_read);
          g_mutex_unlock (&mutex);

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i from queue", entry->tile->x, entry->tile->y, entry->tile->z);

          return;
        }

      g_mutex_unlock (&mutex);
    }

  if (self->in_offset != offset)
    {
      if (lseek (self->i, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      self->in_offset = offset;
    }

  while (to_be_read > 0)
    {
      GError *error = NULL;
      gint    byte_read;

      byte_read = read (self->i, dest + tile_size - to_be_read, to_be_read);
      if (byte_read <= 0)
        {
          g_message ("unable to read tile data from self: "
                     "%s (%d/%d bytes read) %s",
                     g_strerror (errno), byte_read, to_be_read, error?error->message:"--");
          return;
        }
      to_be_read      -= byte_read;
      self->in_offset += byte_read;
    }

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read entry %i,%i,%i at %i", entry->tile->x, entry->tile->y, entry->tile->z, (gint)offset);
}

static inline void
gegl_tile_backend_file_entry_write (GeglTileBackendFile  *self,
                                    GeglFileBackendEntry *entry,
                                    guchar               *source)
{
  GeglFileBackendThreadParams *params;
  gint    length = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  guchar *new_source;

  gegl_tile_backend_file_ensure_exist (self);

  if (entry->tile_link)
    {
      g_mutex_lock (&mutex);

      if (entry->tile_link)
        {
          params = entry->tile_link->data;
          memcpy (params->source, source, length);
          g_mutex_unlock (&mutex);

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "overwrote queue entry %i,%i,%i at %i", entry->tile->x, entry->tile->y, entry->tile->z, (gint)entry->tile->offset);

          return;
        }

      g_mutex_unlock (&mutex);
    }

  new_source = g_malloc (length);
  memcpy (new_source, source, length);

  params            = g_new0 (GeglFileBackendThreadParams, 1);
  params->operation = OP_WRITE;
  params->length    = length;
  params->offset    = entry->tile->offset;
  params->file      = self;
  params->source    = new_source;
  params->entry     = entry;

  gegl_tile_backend_file_push_queue (params);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed entry write %i,%i,%i at %i", entry->tile->x, entry->tile->y, entry->tile->z, (gint)entry->tile->offset);
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
      guint64 offset = *(guint64*)self->free_list->data;

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

          self->total       = self->total + 32 * tile_size;
          params->operation = OP_TRUNCATE;
          params->file      = self;
          params->length    = self->total;

          gegl_tile_backend_file_push_queue (params);

          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed truncate to %i bytes", (gint)self->total);

          self->in_offset = self->out_offset = -1;
        }
    }
  gegl_tile_backend_file_dbg_alloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
  return entry;
}

static void
gegl_tile_backend_file_file_entry_destroy (GeglTileBackendFile  *self,
                                           GeglFileBackendEntry *entry)
{
  guint64 *offset = g_new (guint64, 1);
  *offset = entry->tile->offset;

  if (entry->tile_link || entry->block_link)
    {
      gint   i;
      GList *link;

      g_mutex_lock (&mutex);

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

      g_mutex_unlock (&mutex);
    }

  self->free_list = g_slist_prepend (self->free_list, offset);
  g_hash_table_remove (self->index, entry);

  gegl_tile_backend_file_dbg_dealloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));

  g_free (entry->tile);
  g_free (entry);
}

static gboolean
gegl_tile_backend_file_write_header (GeglTileBackendFile *self)
{
  GeglFileBackendThreadParams *params = g_new0 (GeglFileBackendThreadParams, 1);
  guchar *new_source = g_malloc (256);
  GeglRectangle roi = gegl_tile_backend_get_extent ((GeglTileBackend *)self);

  gegl_tile_backend_file_ensure_exist (self);

  self->header.x = roi.x;
  self->header.y = roi.y;
  self->header.width = roi.width;
  self->header.height = roi.height;

  memcpy (new_source, &(self->header), 256);

  params->operation = OP_WRITE;
  params->source    = new_source;
  params->offset    = 0;
  params->length    = 256;
  params->file      = self;

  gegl_tile_backend_file_push_queue (params);
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed header write, next=%i", (gint)self->header.next);

  params            = g_new0 (GeglFileBackendThreadParams, 1);
  params->operation = OP_SYNC;
  params->file      = self;

  gegl_tile_backend_file_push_queue (params);
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed sync of %s", self->path);

  return TRUE;
}

static gboolean
gegl_tile_backend_file_write_block (GeglTileBackendFile  *self,
                                    GeglFileBackendEntry *item)
{
  gegl_tile_backend_file_ensure_exist (self);
  if (self->in_holding)
    {
      GeglFileBackendThreadParams *params;
      GeglBufferBlock *block           = &(self->in_holding->tile->block);
      guint64          next_allocation = self->offset + block->length;
      gint             length          = block->length;
      guchar          *new_source;

      /* update the next offset pointer in the previous block */
      if (item == NULL)
          /* the previous block was the last block */
          block->next = 0;
      else
          block->next = next_allocation;

      /* XXX: should promiscuosuly try to compress here as well,. if revisions
              are not matching..
       */

      if (self->in_holding->block_link)
        {
          g_mutex_lock (&mutex);

          if (self->in_holding->block_link)
            {
              params = self->in_holding->block_link->data;
              params->offset = self->offset;
              memcpy (params->source, block, length);
              g_mutex_unlock (&mutex);

              self->offset = next_allocation;
              self->in_holding = item;
              GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Overwrote queue block: length:%i flags:%i next:%i at offset %i",
                         block->length,
                         block->flags,
                         (gint)block->next,
                         (gint)self->offset);
              return TRUE;
            }

          g_mutex_unlock (&mutex);
        }

      params     = g_new0 (GeglFileBackendThreadParams, 1);
      new_source = g_malloc (length);

      memcpy (new_source, block, length);

      params->operation = OP_WRITE_BLOCK;
      params->length    = length;
      params->file      = self;
      params->offset    = self->offset;
      params->source    = new_source;
      params->entry     = self->in_holding;

      gegl_tile_backend_file_push_queue (params);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "Pushed write of block: length:%i flags:%i next:%i at offset %i",
                 block->length,
                 block->flags,
                 (gint)block->next,
                 (gint)self->offset);

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

  gegl_tile_backend_file_entry_read (tile_backend_file, entry, gegl_tile_get_data (tile));
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

  gegl_tile_backend_file_entry_write (tile_backend_file, entry, gegl_tile_get_data (tile));
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
      gegl_tile_backend_file_file_entry_destroy (tile_backend_file, entry);
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
gegl_tile_backend_file_free_free_list (GeglTileBackendFile *self)
{
  GSList *iter = self->free_list;

  for (; iter; iter = iter->next)
    g_free (iter->data);

  g_slist_free (self->free_list);

  self->free_list = NULL;
}

static void
gegl_tile_backend_file_finalize (GObject *object)
{
  GeglTileBackendFile *self = (GeglTileBackendFile *) object;

  if (self->index)
    {
      GList *tiles = g_hash_table_get_keys (self->index);

      if (tiles != NULL)
        {
          GList *iter;

          for (iter = tiles; iter; iter = iter->next)
            gegl_tile_backend_file_file_entry_destroy (self, iter->data);
        }

      g_list_free (tiles);

      g_hash_table_unref (self->index);
    }

  if (self->exist)
    {
      gegl_tile_backend_file_finish_writing (self);
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

  if (self->free_list)
    gegl_tile_backend_file_free_free_list (self);

  if (self->path)
    {
      gegl_tile_backend_unlink_swap (self->path);
      g_free (self->path);
    }

  if (self->monitor)
    g_object_unref (self->monitor);

  if (self->file)
    g_object_unref (self->file);

  g_cond_clear (&self->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      self->header = new_header;
      GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "loading index: %s", self->path);
    }

  tile_size       = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  offset          = self->header.next;
  self->tiles     = gegl_buffer_read_index (self->i, &offset);
  self->in_offset = self->out_offset = -1;
  backend         = GEGL_TILE_BACKEND (self);

  for (iter = self->tiles; iter; iter=iter->next)
    {
      GeglBufferItem       *item     = iter->data;
      GeglFileBackendEntry *new;
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
              g_free (existing->tile);
              g_free (existing);

              g_signal_emit_by_name (storage, "changed", &rect, NULL);
            }
        }
      new = gegl_tile_backend_file_file_entry_create (0, 0, 0);
      new->tile = iter->data;
      g_hash_table_insert (self->index, new, new);
    }
  g_list_free (self->tiles);
  gegl_tile_backend_file_free_free_list (self);
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
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    {
      gegl_tile_backend_file_load_index (self, TRUE);
      self->in_offset = self->out_offset = -1;
    }
}

static void
gegl_tile_backend_file_constructed (GObject *object)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);
  GeglTileBackend     *backend = GEGL_TILE_BACKEND (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing file backend: %s", self->path);

  self->file        = g_file_new_for_commandline_arg (self->path);
  self->i           = self->o = -1;
  self->index       = g_hash_table_new (gegl_tile_backend_file_hashfunc,
                                        gegl_tile_backend_file_equalfunc);
  self->pending_ops = 0;
  g_cond_init (&self->cond);

  /* If the file already exists open it, assuming it is a GeglBuffer. */
  if (g_access (self->path, F_OK) != -1)
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
      self->i = g_open (self->path, O_RDONLY, 0);

      self->header     = gegl_buffer_read_header (self->i, &offset)->header;
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
      backend->priv->extent      = (GeglRectangle) {self->header.x,
                                                    self->header.y,
                                                    self->header.width,
                                                    self->header.height};

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

  gegl_tile_backend_set_flush_on_destroy (backend, FALSE);
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

      self->next_pre_alloc = 256; /* reserved space for header */
      self->total          = 256; /* reserved space for header */
      self->in_offset      = self->out_offset = 0;
      self->pending_ops = 0;
      gegl_buffer_header_init (&self->header,
                               backend->priv->tile_width,
                               backend->priv->tile_height,
                               backend->priv->px_size,
                               backend->priv->format);
      gegl_tile_backend_file_write_header (self);
      self->i = g_open (self->path, O_RDONLY, 0);

      g_assert (self->i != -1);
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
  gobject_class->constructed  = gegl_tile_backend_file_constructed;
  gobject_class->finalize     = gegl_tile_backend_file_finalize;

  g_cond_init (&queue_cond);
  g_cond_init (&max_cond);
  g_mutex_init (&mutex);
  g_thread_new ("GeglTileBackendFile async writer thread",
                gegl_tile_backend_file_writer_thread, NULL);

  GEGL_BUFFER_STRUCT_CHECK_PADDING;

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_backend_file_init (GeglTileBackendFile *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_backend_file_command;
  self->path                       = NULL;
  self->file                       = NULL;
  self->i                          = -1;
  self->o                          = -1;
  self->index                      = NULL;
  self->free_list                  = NULL;
  self->next_pre_alloc             = 256; /* reserved space for header */
  self->total                      = 256; /* reserved space for header */
  self->pending_ops                = 0;
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
