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
 *           2012, 2013 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>

#ifdef G_OS_WIN32
#include <process.h>
#define getpid() _getpid()
#endif

#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "gegl.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-backend.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-swap.h"
#include "gegl-debug.h"
#include "gegl-config.h"


#ifndef HAVE_FSYNC

#ifdef G_OS_WIN32
#define fsync _commit
#endif

#endif


G_DEFINE_TYPE (GeglTileBackendSwap, gegl_tile_backend_swap, GEGL_TYPE_TILE_BACKEND)

static GObjectClass * parent_class = NULL;


typedef enum
{
  OP_WRITE,
  OP_TRUNCATE,
} ThreadOp;

typedef struct
{
  guint64  offset;
  GList   *link;
  gint     x;
  gint     y;
  gint     z;
} SwapEntry;

typedef struct
{
  SwapEntry *entry;
  gint       length;
  GeglTile  *tile;
  ThreadOp   operation;
} ThreadParams;

typedef struct
{
  guint64 start;
  guint64 end;
} SwapGap;


static void        gegl_tile_backend_swap_push_queue    (ThreadParams *params);
static void        gegl_tile_backend_swap_write         (ThreadParams *params);
static gpointer    gegl_tile_backend_swap_writer_thread (gpointer ignored);
static void        gegl_tile_backend_swap_entry_read    (GeglTileBackendSwap   *self,
                                                         SwapEntry             *entry,
                                                         guchar                *dest);
static void        gegl_tile_backend_swap_entry_write   (GeglTileBackendSwap   *self,
                                                         SwapEntry             *entry,
                                                         GeglTile              *tile);
static SwapEntry * gegl_tile_backend_swap_entry_create  (gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static guint64     gegl_tile_backend_swap_find_offset   (gint                   tile_size);
static SwapGap *   gegl_tile_backend_swap_gap_new       (guint64                start,
                                                         guint64                end);
static void        gegl_tile_backend_swap_entry_destroy (GeglTileBackendSwap   *self,
                                                         SwapEntry             *entry);
static void        gegl_tile_backend_swap_resize        (guint64 size);
static SwapEntry * gegl_tile_backend_swap_lookup_entry  (GeglTileBackendSwap   *self,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static GeglTile *  gegl_tile_backend_swap_get_tile      (GeglTileSource        *self,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static gpointer    gegl_tile_backend_swap_set_tile      (GeglTileSource        *self,
                                                         GeglTile              *tile,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static gpointer    gegl_tile_backend_swap_void_tile     (GeglTileSource        *self,
                                                         GeglTile              *tile,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static gpointer    gegl_tile_backend_swap_exist_tile    (GeglTileSource        *self,
                                                         GeglTile              *tile,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z);
static gpointer    gegl_tile_backend_swap_command       (GeglTileSource        *self,
                                                         GeglTileCommand        command,
                                                         gint                   x,
                                                         gint                   y,
                                                         gint                   z,
                                                         gpointer               data);
static guint       gegl_tile_backend_swap_hashfunc      (gconstpointer key);
static gboolean    gegl_tile_backend_swap_equalfunc     (gconstpointer          a,
                                                         gconstpointer          b);
static void        gegl_tile_backend_swap_constructed   (GObject *object);
static void        gegl_tile_backend_swap_finalize      (GObject *object);
static void        gegl_tile_backend_swap_ensure_exist  (void);
static void        gegl_tile_backend_swap_class_init    (GeglTileBackendSwapClass *klass);
static void        gegl_tile_backend_swap_init          (GeglTileBackendSwap *self);
void               gegl_tile_backend_swap_cleanup       (void);


static gchar   *path       = NULL;
static gint     in_fd      = -1;
static gint     out_fd     = -1;
static guint64  in_offset  = 0;
static guint64  out_offset = 0;
static GList   *gap_list   = NULL;
static guint64  total      = 0;

static GThread      *writer_thread = NULL;
static GQueue       *queue         = NULL;
static ThreadParams *in_progress   = NULL;
static gboolean      exit_thread   = FALSE;
static GMutex        mutex;
static GCond         queue_cond;


static void
gegl_tile_backend_swap_push_queue (ThreadParams *params)
{
  g_mutex_lock (&mutex);

  g_queue_push_tail (queue, params);

  if (params->operation == OP_WRITE)
    params->entry->link = g_queue_peek_tail_link (queue);

  /* wake up the writer thread */
  g_cond_signal (&queue_cond);

  g_mutex_unlock (&mutex);
}

static void
gegl_tile_backend_swap_write (ThreadParams *params)
{
  gint    to_be_written = params->length;
  guint64 offset        = params->entry->offset;

  if (out_offset != offset)
    {
      if (lseek (out_fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      out_offset = offset;
    }

  while (to_be_written > 0)
    {
      gint wrote;
      wrote = write (out_fd,
                     gegl_tile_get_data (params->tile) + params->length
                     - to_be_written,
                     to_be_written);
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          break;
        }

      to_be_written -= wrote;
      out_offset    += wrote;
    }

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "writer thread wrote at %i", (gint)offset);
}

static gpointer
gegl_tile_backend_swap_writer_thread (gpointer ignored)
{
  while (TRUE)
    {
      ThreadParams *params;

      g_mutex_lock (&mutex);

      while (g_queue_is_empty (queue) && !exit_thread)
        g_cond_wait (&queue_cond, &mutex);

      if (exit_thread)
        {
          g_mutex_unlock (&mutex);
          GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "exiting writer thread");
          return NULL;
        }

      params = (ThreadParams *)g_queue_pop_head (queue);
      if (params->operation == OP_WRITE)
        {
          in_progress = params;
          params->entry->link = NULL;
        }

      g_mutex_unlock (&mutex);

      switch (params->operation)
        {
        case OP_WRITE:
          gegl_tile_backend_swap_write (params);
          break;
        case OP_TRUNCATE:
          if (ftruncate (out_fd, total) != 0)
            g_warning ("failed to resize swap file: %s", g_strerror (errno));
          break;
        }

      g_mutex_lock (&mutex);

      in_progress = NULL;

      if (params->operation == OP_WRITE)
        gegl_tile_unref (params->tile);

      g_slice_free (ThreadParams, params);

      g_mutex_unlock (&mutex);
    }

  return NULL;
}

static void
gegl_tile_backend_swap_entry_read (GeglTileBackendSwap *self,
                                   SwapEntry           *entry,
                                   guchar              *dest)
{
  gint    tile_size  = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  gint    to_be_read = tile_size;
  guint64 offset     = entry->offset;

  gegl_tile_backend_swap_ensure_exist ();

  if (entry->link || in_progress)
    {
      ThreadParams *queued_op = NULL;
      g_mutex_lock (&mutex);

      if (entry->link)
        queued_op = entry->link->data;
      else if (in_progress && in_progress->entry == entry)
        queued_op = in_progress;

      if (queued_op)
        {
          memcpy (dest, gegl_tile_get_data (queued_op->tile), to_be_read);
          g_mutex_unlock (&mutex);

          GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "read entry %i, %i, %i from queue", entry->x, entry->y, entry->z);

          return;
        }

      g_mutex_unlock (&mutex);
    }

  if (in_offset != offset)
    {
      if (lseek (in_fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      in_offset = offset;
    }

  while (to_be_read > 0)
    {
      GError *error = NULL;
      gint    byte_read;

      byte_read = read (in_fd, dest + tile_size - to_be_read, to_be_read);
      if (byte_read <= 0)
        {
          g_message ("unable to read tile data from swap: "
                     "%s (%d/%d bytes read) %s",
                     g_strerror (errno), byte_read, to_be_read, error?error->message:"--");
          return;
        }
      to_be_read -= byte_read;
      in_offset  += byte_read;
    }

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "read entry %i, %i, %i from %i", entry->x, entry->y, entry->z, (gint)offset);
}

static void
gegl_tile_backend_swap_entry_write (GeglTileBackendSwap *self,
                                    SwapEntry           *entry,
                                    GeglTile            *tile)
{
  ThreadParams *params;
  gint          length = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));

  gegl_tile_backend_swap_ensure_exist ();

  if (entry->link)
    {
      g_mutex_lock (&mutex);

      if (entry->link)
        {
          params = entry->link->data;
          gegl_tile_unref (params->tile);
          params->tile = gegl_tile_dup (tile);
          g_mutex_unlock (&mutex);

          GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "tile %i, %i, %i at %i is already enqueued, changed data", entry->x, entry->y, entry->z, (gint)entry->offset);

          return;
        }

      g_mutex_unlock (&mutex);
    }

  params            = g_slice_new0 (ThreadParams);
  params->operation = OP_WRITE;
  params->length    = length;
  params->tile      = gegl_tile_dup (tile);
  params->entry     = entry;

  gegl_tile_backend_swap_push_queue (params);

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "pushed write of entry %i, %i, %i at %i", entry->x, entry->y, entry->z, (gint)entry->offset);
}

static SwapEntry *
gegl_tile_backend_swap_entry_create (gint x,
                                     gint y,
                                     gint z)
{
  SwapEntry *entry = g_slice_new0 (SwapEntry);

  entry->x    = x;
  entry->y    = y;
  entry->z    = z;
  entry->link = NULL;

  return entry;
}

static guint64
gegl_tile_backend_swap_find_offset (gint tile_size)
{
  SwapGap *gap;
  guint64  offset;

  if (gap_list)
    {
      GList *link = gap_list;

      while (link)
        {
          guint64 length;

          gap    = link->data;
          length = gap->end - gap->start;

          if (length > tile_size)
            {
              offset = gap->start;
              gap->start += tile_size;

              return offset;
            }
          else if (length == tile_size)
            {
              offset = gap->start;
              g_slice_free (SwapGap, gap);
              gap_list = g_list_delete_link (gap_list, link);

              return offset;
            }

          link = link->next;
        }
    }

  offset = total;

  gegl_tile_backend_swap_resize (total + 32 * tile_size);

  gap = gegl_tile_backend_swap_gap_new (offset + tile_size, total);
  gap_list = g_list_append (gap_list, gap);

  return offset;
}

static SwapGap *
gegl_tile_backend_swap_gap_new (guint64 start,
                                guint64 end)
{
  SwapGap *gap = g_slice_new (SwapGap);

  gap->start = start;
  gap->end = end;

  return gap;
}

static void
gegl_tile_backend_swap_entry_destroy (GeglTileBackendSwap *self,
                                      SwapEntry           *entry)
{
  guint64  start, end;
  gint     tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  GList   *hlink;

  if (entry->link)
    {
      GList *link;

      g_mutex_lock (&mutex);

      if ((link = entry->link))
        {
          ThreadParams *queued_op = link->data;
          g_queue_delete_link (queue, link);
          gegl_tile_unref (queued_op->tile);
          g_slice_free (ThreadParams, queued_op);
        }

      g_mutex_unlock (&mutex);
    }

  start = entry->offset;
  end = start + tile_size;

  if ((hlink = gap_list))
    while (hlink)
      {
        GList *llink = hlink->prev;
        SwapGap *lgap, *hgap;

        if (llink)
          lgap = llink->data;

        hgap = hlink->data;

        /* attempt to merge lower, upper and this gap */
        if (llink != NULL && lgap->end == start &&
            hgap->start == end)
          {
            llink->next = hlink->next;
            if (hlink->next)
              hlink->next->prev = llink;
            lgap->end = hgap->end;

            g_slice_free (SwapGap, hgap);
            hlink->next = NULL;
            hlink->prev = NULL;
            g_list_free (hlink);
            break;
          }
        /* attempt to merge with upper gap */
        else if (hgap->start == end)
          {
            hgap->start = start;
            break;
          }
        /* attempt to merge with lower gap */
        else if (llink != NULL && lgap->end == start)
          {
            lgap->end = end;
            break;
          }
        /* create new gap */
        else if (hgap->start > end)
          {
            lgap = gegl_tile_backend_swap_gap_new (start, end);
            gap_list = g_list_insert_before (gap_list, hlink, lgap);
            break;
          }

        /* if there's no more elements in the list after this */
        else if (hlink->next == NULL)
          {
            /* attempt to merge with the last gap */
            if (hgap->end == start)
              {
                hgap->end = end;
              }
            /* create a new gap in the end of the list */
            else
              {
                GList *new_link;
                hgap = gegl_tile_backend_swap_gap_new (start, end);
                new_link = g_list_alloc ();
                new_link->next = NULL;
                new_link->prev = hlink;
                new_link->data = hgap;
                hlink->next = new_link;
              }
            break;
          }

        hlink = hlink->next;
      }
  else
    gap_list = g_list_prepend (NULL,
                               gegl_tile_backend_swap_gap_new (start, end));

  g_hash_table_remove (self->index, entry);
  g_slice_free (SwapEntry, entry);
}

static void
gegl_tile_backend_swap_resize (guint64 size)
{
  ThreadParams *params;

  total = size;
  params = g_slice_new0 (ThreadParams);
  params->operation = OP_TRUNCATE;

  gegl_tile_backend_swap_push_queue (params);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed resize to %i", (gint)total);
}

static SwapEntry *
gegl_tile_backend_swap_lookup_entry (GeglTileBackendSwap *self,
                                     gint                 x,
                                     gint                 y,
                                     gint                 z)
{
  SwapEntry key = {0, NULL, x, y, z};

  return g_hash_table_lookup (self->index, &key);
}

static GeglTile *
gegl_tile_backend_swap_get_tile (GeglTileSource *self,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackendSwap *tile_backend_swap;
  SwapEntry           *entry;
  GeglTile            *tile = NULL;
  gint                 tile_size;

  tile_backend_swap = GEGL_TILE_BACKEND_SWAP (self);
  entry             = gegl_tile_backend_swap_lookup_entry (tile_backend_swap, x, y, z);

  if (!entry)
    return NULL;

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  tile      = gegl_tile_new (tile_size);
  gegl_tile_mark_as_stored (tile);

  gegl_tile_backend_swap_entry_read (tile_backend_swap, entry, gegl_tile_get_data (tile));

  return tile;
}

static gpointer
gegl_tile_backend_swap_set_tile (GeglTileSource *self,
                                 GeglTile       *tile,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendSwap *tile_backend_swap;
  SwapEntry           *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_swap = GEGL_TILE_BACKEND_SWAP (backend);
  entry             = gegl_tile_backend_swap_lookup_entry (tile_backend_swap, x, y, z);

  gegl_tile_backend_swap_ensure_exist ();

  if (entry == NULL)
    {
      entry          = gegl_tile_backend_swap_entry_create (x, y, z);
      entry->offset  = gegl_tile_backend_swap_find_offset (gegl_tile_backend_get_tile_size (backend));
      g_hash_table_insert (tile_backend_swap->index, entry, entry);
    }

  gegl_tile_backend_swap_entry_write (tile_backend_swap, entry, tile);

  gegl_tile_mark_as_stored (tile);

  return NULL;
}

static gpointer
gegl_tile_backend_swap_void_tile (GeglTileSource *self,
                                  GeglTile       *tile,
                                  gint            x,
                                  gint            y,
                                  gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendSwap *tile_backend_swap;
  SwapEntry           *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_swap = GEGL_TILE_BACKEND_SWAP (backend);
  entry             = gegl_tile_backend_swap_lookup_entry (tile_backend_swap, x, y, z);

  if (entry != NULL)
    {
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "void tile %i, %i, %i", x, y, z);

      gegl_tile_backend_swap_entry_destroy (tile_backend_swap, entry);
    }

  return NULL;
}

static gpointer
gegl_tile_backend_swap_exist_tile (GeglTileSource *self,
                                   GeglTile       *tile,
                                   gint            x,
                                   gint            y,
                                   gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendSwap *tile_backend_swap;
  SwapEntry           *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_swap = GEGL_TILE_BACKEND_SWAP (backend);
  entry             = gegl_tile_backend_swap_lookup_entry (tile_backend_swap, x, y, z);

  return entry!=NULL?((gpointer)0x1):NULL;
}

static gpointer
gegl_tile_backend_swap_command (GeglTileSource  *self,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return gegl_tile_backend_swap_get_tile (self, x, y, z);
      case GEGL_TILE_SET:
        return gegl_tile_backend_swap_set_tile (self, data, x, y, z);
      case GEGL_TILE_IDLE:
        return NULL;
      case GEGL_TILE_VOID:
        return gegl_tile_backend_swap_void_tile (self, data, x, y, z);
      case GEGL_TILE_EXIST:
        return gegl_tile_backend_swap_exist_tile (self, data, x, y, z);
      case GEGL_TILE_FLUSH:
        return NULL;

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return FALSE;
}

static guint
gegl_tile_backend_swap_hashfunc (gconstpointer key)
{
  const SwapEntry *entry = key;
  guint            hash;
  gint             i;
  gint             srcA  = entry->x;
  gint             srcB  = entry->y;
  gint             srcC  = entry->z;

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
gegl_tile_backend_swap_equalfunc (gconstpointer a,
                                  gconstpointer b)
{
  const SwapEntry *ea = a;
  const SwapEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}

static void
gegl_tile_backend_swap_constructed (GObject *object)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gegl_tile_backend_set_flush_on_destroy (backend, FALSE);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing swap backend");
}

static void
gegl_tile_backend_swap_finalize (GObject *object)
{
  GeglTileBackendSwap *self = GEGL_TILE_BACKEND_SWAP (object);

  if (self->index)
    {
      GList *tiles = g_hash_table_get_keys (self->index);

      if (tiles != NULL)
        {
          GList *iter;

          for (iter = tiles; iter; iter = iter->next)
            gegl_tile_backend_swap_entry_destroy (self, iter->data);
        }

      g_list_free (tiles);

      g_hash_table_unref (self->index);

      self->index = NULL;
    }

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_tile_backend_swap_ensure_exist (void)
{
  if (in_fd == -1 || out_fd == -1)
    {
      gchar *filename = g_strdup_printf ("%i-shared.swap", getpid ());
      path = g_build_filename (gegl_config ()->swap, filename, NULL);
      g_free (filename);

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "creating swapfile %s", path);

      out_fd = g_open (path, O_RDWR|O_CREAT, 0770);
      in_fd = g_open (path, O_RDONLY, 0);

      if (out_fd == -1 || in_fd == -1)
        g_warning ("Could not open swap file '%s': %s", path, g_strerror (errno));
    }
}

static void
gegl_tile_backend_swap_class_init (GeglTileBackendSwapClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->constructed  = gegl_tile_backend_swap_constructed;
  gobject_class->finalize     = gegl_tile_backend_swap_finalize;

  queue         = g_queue_new ();
  writer_thread = g_thread_new ("swap writer",
                                gegl_tile_backend_swap_writer_thread,
                                NULL);
}

void
gegl_tile_backend_swap_cleanup (void)
{
  if (in_fd != -1 && out_fd != -1)
    {
      exit_thread = TRUE;
      g_cond_signal (&queue_cond);
      g_thread_join (writer_thread);

      if (g_queue_get_length (queue) != 0)
        g_warning ("tile-backend-swap writer queue wasn't empty before freeing\n");

      g_queue_free (queue);

      if (gap_list)
        {
          SwapGap *gap = gap_list->data;

          if (gap_list->next)
            g_warning ("tile-backend-swap gap list had more than one element\n");

          g_warn_if_fail (gap->start == 0 && gap->end == total);

          g_slice_free (SwapGap, gap_list->data);
          g_list_free (gap_list);
        }
      else
        g_warn_if_fail (total == 0);

      close (in_fd);
      close (out_fd);

      in_fd = out_fd = -1;
    }
}

static void
gegl_tile_backend_swap_init (GeglTileBackendSwap *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_backend_swap_command;

  self->index = g_hash_table_new (gegl_tile_backend_swap_hashfunc,
                                  gegl_tile_backend_swap_equalfunc);
}
