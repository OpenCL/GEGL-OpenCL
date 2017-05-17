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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"
#include "gegl-debug.h"

#include "gegl-buffer-cl-cache.h"

/*
#define GEGL_DEBUG_CACHE_HITS
*/

typedef struct CacheItem
{
  GeglTileHandlerCache *handler; /* The specific handler that cached this item*/
  GeglTile *tile;                /* The tile */
  GList     link;                /*  Link in the cache_queue, to avoid
                                  *  queue lookups involving g_list_find() */

  gint      x;                   /* The coordinates this tile was cached for */
  gint      y;
  gint      z;
} CacheItem;

#define LINK_GET_ITEM(link) \
        ((CacheItem *) ((guchar *) link - G_STRUCT_OFFSET (CacheItem, link)))


static gboolean   gegl_tile_handler_cache_equalfunc  (gconstpointer         a,
                                                      gconstpointer         b);
static guint      gegl_tile_handler_cache_hashfunc   (gconstpointer         key);
static void       gegl_tile_handler_cache_dispose    (GObject              *object);
static gboolean   gegl_tile_handler_cache_wash       (GeglTileHandlerCache *cache);
static gpointer   gegl_tile_handler_cache_command    (GeglTileSource       *tile_store,
                                                      GeglTileCommand       command,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z,
                                                      gpointer              data);
static GeglTile * gegl_tile_handler_cache_get_tile   (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static gboolean   gegl_tile_handler_cache_has_tile   (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
void              gegl_tile_handler_cache_insert     (GeglTileHandlerCache *cache,
                                                      GeglTile             *tile,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static void       gegl_tile_handler_cache_void       (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static void       gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);


static GMutex       mutex                 = { 0, };
static GQueue      *cache_queue           = NULL;
static gint         cache_wash_percentage = 20;
static guint64      cache_total           = 0; /* approximate amount of bytes stored */
#ifdef GEGL_DEBUG_CACHE_HITS
static gint         cache_hits            = 0;
static gint         cache_misses          = 0;
#endif


G_DEFINE_TYPE (GeglTileHandlerCache, gegl_tile_handler_cache, GEGL_TYPE_TILE_HANDLER)


static void
gegl_tile_handler_cache_class_init (GeglTileHandlerCacheClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = gegl_tile_handler_cache_dispose;
}

static void
gegl_tile_handler_cache_init (GeglTileHandlerCache *cache)
{
  ((GeglTileSource*)cache)->command = gegl_tile_handler_cache_command;
  cache->items = g_hash_table_new (gegl_tile_handler_cache_hashfunc, gegl_tile_handler_cache_equalfunc);
  gegl_tile_cache_init ();
}

static void
gegl_tile_handler_cache_reinit (GeglTileHandlerCache *cache)
{
  CacheItem            *item;
  GHashTableIter        iter;
  gpointer              key, value;

  if (cache->tile_storage->hot_tile)
    {
      gegl_tile_unref (cache->tile_storage->hot_tile);
      cache->tile_storage->hot_tile = NULL;
    }

  if (!cache->count)
    return;

  g_mutex_lock (&mutex);
  {
    g_hash_table_iter_init (&iter, cache->items);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
      item = (CacheItem *) value;
      if (item->tile)
        {
          cache_total -= item->tile->size;
          gegl_tile_mark_as_stored (item->tile); // to avoid saving 
          gegl_tile_unref (item->tile);
          cache->count--;
        }
      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_iter_remove (&iter);
      g_slice_free (CacheItem, item);
    }
  }

  g_mutex_unlock (&mutex);
}

static void
gegl_tile_handler_cache_dispose (GObject *object)
{
  GeglTileHandlerCache *cache = GEGL_TILE_HANDLER_CACHE (object);

  gegl_tile_handler_cache_reinit (cache);

  if (cache->count < 0)
    {
      g_warning ("cache-handler tile balance not zero: %i\n", cache->count);
    }

  g_hash_table_destroy (cache->items);
  G_OBJECT_CLASS (gegl_tile_handler_cache_parent_class)->dispose (object);
}

static GeglTile *
gegl_tile_handler_cache_get_tile_command (GeglTileSource *tile_store,
                                          gint        x,
                                          gint        y,
                                          gint        z)
{
  GeglTileHandlerCache *cache    = (GeglTileHandlerCache*) (tile_store);
  GeglTileSource       *source   = ((GeglTileHandler*) (tile_store))->source;
  GeglTile             *tile     = NULL;

  if (G_UNLIKELY (gegl_cl_is_accelerated ()))
    gegl_buffer_cl_cache_flush2 (cache, NULL);

  tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);
  if (tile)
    {
#ifdef GEGL_DEBUG_CACHE_HITS
      cache_hits++;
#endif
      return tile;
    }
#ifdef GEGL_DEBUG_CACHE_HITS
  cache_misses++;
#endif

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);

  if (tile)
    gegl_tile_handler_cache_insert (cache, tile, x, y, z);

  return tile;
}

static gpointer
gegl_tile_handler_cache_command (GeglTileSource  *tile_store,
                                 GeglTileCommand  command,
                                 gint             x,
                                 gint             y,
                                 gint             z,
                                 gpointer         data)
{
  GeglTileHandler      *handler = (GeglTileHandler*) (tile_store);
  GeglTileHandlerCache *cache   = (GeglTileHandlerCache*) (handler);

  switch (command)
    {
      case GEGL_TILE_FLUSH:
        {
          GList     *link;

          if (gegl_cl_is_accelerated ())
            gegl_buffer_cl_cache_flush2 (cache, NULL);

          if (cache->count)
            {
              for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
                {
                  CacheItem *item = LINK_GET_ITEM (link);
                  GeglTile  *tile = item->tile;

                  if (tile != NULL &&
                      item->handler == cache)
                    {
                      gegl_tile_store (tile);
                    }
                }
            }
        }
        break;
      case GEGL_TILE_GET:
        /* XXX: we should perhaps store a NIL result, and place the empty
         * generator after the cache, this would have to be possible to disable
         * to work in sync operation with backend.
         */
        return gegl_tile_handler_cache_get_tile_command (tile_store, x, y, z);
      case GEGL_TILE_IS_CACHED:
        return GINT_TO_POINTER(gegl_tile_handler_cache_has_tile (cache, x, y, z));
      case GEGL_TILE_EXIST:
        {
          gboolean exist = gegl_tile_handler_cache_has_tile (cache, x, y, z);
          if (exist)
            return (gpointer)TRUE;
        }
        break;
      case GEGL_TILE_IDLE:
        {
          gboolean action = gegl_tile_handler_cache_wash (cache);
          if (action)
            return GINT_TO_POINTER(action);
          /* with no action, we chain up to lower levels */
          break;
        }
      case GEGL_TILE_REFETCH:
        gegl_tile_handler_cache_invalidate (cache, x, y, z);
        break;
      case GEGL_TILE_VOID:
        gegl_tile_handler_cache_void (cache, x, y, z);
        break;
      case GEGL_TILE_REINIT:
        gegl_tile_handler_cache_reinit (cache);
        break;
      default:
        break;
    }

  return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

/* write the least recently used dirty tile to disk if it
 * is in the wash_percentage (20%) least recently used tiles,
 * calling this function in an idle handler distributes the
 * tile flushing overhead over time.
 */
gboolean
gegl_tile_handler_cache_wash (GeglTileHandlerCache *cache)
{
  GeglTile  *last_dirty = NULL;
  guint      count      = 0;
  gint       length     = g_queue_get_length (cache_queue);
  gint       wash_tiles = cache_wash_percentage * length / 100;
  GList     *link;

  for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
    {
      CacheItem *item = LINK_GET_ITEM (link);
      GeglTile  *tile = item->tile;

      count++;
      if (!gegl_tile_is_stored (tile))
        if (count > length - wash_tiles)
          last_dirty = tile;
    }

  if (last_dirty != NULL)
    {
      gegl_tile_store (last_dirty);
      return TRUE;
    }
  return FALSE;
}

static inline CacheItem *
cache_lookup (GeglTileHandlerCache *cache,
              gint                  x,
              gint                  y,
              gint                  z)
{
  CacheItem key;

  key.x       = x;
  key.y       = y;
  key.z       = z;
  key.handler = cache;

  return g_hash_table_lookup (cache->items, &key);
}

/* returns the requested Tile if it is in the cache, NULL otherwize.
 */
static GeglTile *
gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  CacheItem *result;

  if (cache->count == 0)
    return NULL;

  g_mutex_lock (&mutex);
  result = cache_lookup (cache, x, y, z);
  if (result)
    {
      g_queue_unlink (cache_queue, &result->link);
      g_queue_push_head_link (cache_queue, &result->link);
      g_mutex_unlock (&mutex);
      while (result->tile == NULL)
      {
        g_printerr ("NULL tile in %s %p %i %i %i %p\n", __FUNCTION__, result, result->x, result->y, result->z,
                result->tile);
        return NULL;
      }
      return gegl_tile_ref (result->tile);
    }
  g_mutex_unlock (&mutex);
  return NULL;
}

static gboolean
gegl_tile_handler_cache_has_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  GeglTile *tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);

  if (tile)
    {
      gegl_tile_unref (tile);
      return TRUE;
    }

  return FALSE;
}

static void
drop_hot_tile (GeglTile *tile)
{
  GeglTileStorage *storage = tile->tile_storage;

  if (storage)
    {
      if (gegl_config_threads()>1)
        g_rec_mutex_lock (&storage->mutex);

      if (storage->hot_tile == tile)
        {
          gegl_tile_unref (storage->hot_tile);
          storage->hot_tile = NULL;
        }

      if (gegl_config_threads()>1)
        g_rec_mutex_unlock (&storage->mutex);
    }
}

static gboolean
gegl_tile_handler_cache_trim (GeglTileHandlerCache *cache)
{
  GList *link;

  link = g_queue_pop_tail_link (cache_queue);

  if (link != NULL)
    {
      CacheItem *last_writable = LINK_GET_ITEM (link);
      GeglTile *tile = last_writable->tile;

      g_hash_table_remove (last_writable->handler->items, last_writable);
      cache_total -= tile->size;
      drop_hot_tile (tile);
      gegl_tile_unref (tile);
      g_slice_free (CacheItem, last_writable);
      return TRUE;
    }

  return FALSE;
}

static void
gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                    gint                  x,
                                    gint                  y,
                                    gint                  z)
{
  CacheItem *item;

  g_mutex_lock (&mutex);
  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      cache_total -= item->tile->size;
      drop_hot_tile (item->tile);
      item->tile->tile_storage = NULL;
      gegl_tile_mark_as_stored (item->tile); /* to cheat it out of being stored */
      gegl_tile_unref (item->tile);

      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_remove (cache->items, item);

      g_slice_free (CacheItem, item);
    }
  g_mutex_unlock (&mutex);
}


static void
gegl_tile_handler_cache_void (GeglTileHandlerCache *cache,
                              gint                  x,
                              gint                  y,
                              gint                  z)
{
  CacheItem *item;

  g_mutex_lock (&mutex);
  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      cache_total -= item->tile->size;
      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_remove (cache->items, item);
      cache->count--;
    }
  g_mutex_unlock (&mutex);

  if (item)
    {
      drop_hot_tile (item->tile);
      gegl_tile_void (item->tile);
      gegl_tile_unref (item->tile);
    }

  g_slice_free (CacheItem, item);
}

void
gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                GeglTile             *tile,
                                gint                  x,
                                gint                  y,
                                gint                  z)
{
  CacheItem *item = g_slice_new (CacheItem);

  item->handler   = cache;
  item->tile      = gegl_tile_ref (tile);
  item->link.data = item;
  item->link.next = NULL;
  item->link.prev = NULL;
  item->x         = x;
  item->y         = y;
  item->z         = z;

  tile->x = x;
  tile->y = y;
  tile->z = z;
  tile->tile_storage = cache->tile_storage;

  // XXX : remove entry if it already exists
  gegl_tile_handler_cache_void (cache, x, y, z);

  /* XXX: this is a window when the tile is a zero tile during update */

  g_mutex_lock (&mutex);
  cache_total  += item->tile->size;
  g_queue_push_head_link (cache_queue, &item->link);

  cache->count ++;

  g_hash_table_insert (cache->items, item, item);

  while (cache_total > gegl_config()->tile_cache_size)
    {
#ifdef GEGL_DEBUG_CACHE_HITS
      GEGL_NOTE(GEGL_DEBUG_CACHE, "cache_total:"G_GUINT64_FORMAT" > cache_size:"G_GUINT64_FORMAT, cache_total, gegl_config()->tile_cache_size);
      GEGL_NOTE(GEGL_DEBUG_CACHE, "%f%% hit:%i miss:%i  %i]", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses, g_queue_get_length (cache_queue));
#endif
      gegl_tile_handler_cache_trim (cache);
    }
  g_mutex_unlock (&mutex);
}

GeglTileHandler *
gegl_tile_handler_cache_new (void)
{
  return g_object_new (GEGL_TYPE_TILE_HANDLER_CACHE, NULL);
}


static guint
gegl_tile_handler_cache_hashfunc (gconstpointer key)
{
  const CacheItem *e = key;
  guint           hash;
  gint            i;
  gint            srcA = e->x;
  gint            srcB = e->y;
  gint            srcC = e->z;

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
  return hash ^ GPOINTER_TO_INT (e->handler);
}

static gboolean
gegl_tile_handler_cache_equalfunc (gconstpointer a,
                                   gconstpointer b)
{
  const CacheItem *ea = a;
  const CacheItem *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z &&
      ea->handler == eb->handler)
    return TRUE;
  return FALSE;
}

void
gegl_tile_cache_init (void)
{
  if (cache_queue == NULL)
    cache_queue = g_queue_new ();
}

void
gegl_tile_cache_destroy (void)
{
  if (cache_queue)
    {
      while (g_queue_pop_head_link (cache_queue));
      g_queue_free (cache_queue);
    }
  cache_queue = NULL;
}
