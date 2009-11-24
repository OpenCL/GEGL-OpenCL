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
#include "gegl-debug.h"

#if ENABLE_MT
static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
#endif

struct _GeglTileHandlerCache
{
  GeglTileHandler parent_instance;
  GSList *free_list;
};


G_DEFINE_TYPE (GeglTileHandlerCache, gegl_tile_handler_cache, GEGL_TYPE_TILE_HANDLER)

typedef struct CacheItem
{ 
  GeglTileHandlerCache *handler; /* The specific handler that cached this item*/
  GeglTile *tile;                /* The tile */

  gint      x;                   /* The coordinates this tile was cached for */
  gint      y;
  gint      z;
} CacheItem;

static GQueue *cache_queue = NULL;
static GHashTable *cache_ht = NULL;
static gint    cache_wash_percentage = 20;
#if 0
static gint    cache_hits = 0;
static gint    cache_misses = 0;
#endif

static gint    cache_total = 0;  /* approximate amount of bytes stored */



static guint hashfunc (gconstpointer key)
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
#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; \
    } \
  while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return hash ^ GPOINTER_TO_INT (e->handler);
}

static gboolean equalfunc (gconstpointer a,
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


void gegl_tile_cache_init (void)
{
  if (cache_queue == NULL)
    cache_queue = g_queue_new ();
  if (cache_ht == NULL)
    cache_ht = g_hash_table_new (hashfunc, equalfunc);
}

void gegl_tile_cache_destroy (void)
{
  if (cache_queue)
    g_queue_free (cache_queue);
  if (cache_ht)
    g_hash_table_destroy (cache_ht);
  cache_queue = NULL;
  cache_ht = NULL;
}

static gboolean    gegl_tile_handler_cache_wash     (GeglTileHandlerCache *cache);

static GeglTile *  gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                                     gint              x,
                                                     gint              y,
                                                     gint              z);

static gboolean    gegl_tile_handler_cache_has_tile (GeglTileHandlerCache *cache,
                                                     gint              x,
                                                     gint              y,
                                                     gint              z);
void               gegl_tile_handler_cache_insert   (GeglTileHandlerCache *cache,
                                                     GeglTile         *tile,
                                                     gint              x,
                                                     gint              y,
                                                     gint              z);
static void        gegl_tile_handler_cache_void     (GeglTileHandlerCache *cache,
                                                     gint              x,
                                                     gint              y,
                                                     gint              z);
static void        gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                                     gint              x,
                                                     gint              y,
                                                     gint              z);


static void
finalize (GObject *object)
{
  GeglTileHandlerCache *cache;
  cache = (GeglTileHandlerCache *) object;

  G_OBJECT_CLASS (gegl_tile_handler_cache_parent_class)->finalize (object);
}

static void
gegl_tile_handler_cache_dispose_buffer_tiles (gpointer itm,
                                              gpointer userdata)
{
  CacheItem *item;
  item = itm;
  if (item->handler == userdata)
    {
      GeglTileHandlerCache *cache = userdata;
      cache->free_list = g_slist_prepend (cache->free_list, item);
      
    }
}

static void
dispose (GObject *object)
{
  GeglTileHandlerCache *cache;
  CacheItem            *item;
  GSList               *iter;

#ifdef ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  cache = GEGL_TILE_HANDLER_CACHE (object);

  /* only throw out items belonging to this cache instance */

  cache->free_list = NULL;
  g_queue_foreach (cache_queue, gegl_tile_handler_cache_dispose_buffer_tiles, cache);
  for (iter = cache->free_list; iter; iter = g_slist_next (iter))
    {
        item = iter->data;
        if (item->tile)
          {
            cache_total -= item->tile->size;
            gegl_tile_unref (item->tile);
          }
        g_queue_remove (cache_queue, item);
        g_hash_table_remove (cache_ht, item);
        g_slice_free (CacheItem, item);
    }
  g_slist_free (cache->free_list);
  cache->free_list = NULL;
#ifdef ENABLE_MT
  g_static_mutex_unlock (&mutex);
#endif

  G_OBJECT_CLASS (gegl_tile_handler_cache_parent_class)->dispose (object);
}

static GeglTile *
get_tile (GeglTileSource *tile_store,
          gint        x,
          gint        y,
          gint        z)
{
  GeglTileHandlerCache *cache    = GEGL_TILE_HANDLER_CACHE (tile_store);
  GeglTileSource       *source = GEGL_HANDLER (tile_store)->source;
  GeglTile             *tile     = NULL;

  tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);
  if (tile)
    {
#if 0
      cache_hits++;
#endif
      return tile;
    }
#if 0
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
  GeglTileHandler      *handler = GEGL_HANDLER (tile_store);
  GeglTileHandlerCache *cache   = GEGL_TILE_HANDLER_CACHE (handler);

  switch (command)
    {
      case GEGL_TILE_FLUSH:
        {
          GList     *link;

          for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
            {
              CacheItem *item = link->data;
              GeglTile  *tile = item->tile;

              if (tile != NULL &&
                  item->handler == cache)
                {
                  gegl_tile_store (tile);
                }
            }
        }
        break;
      case GEGL_TILE_GET:
        /* XXX: we should perhaps store a NIL result, and place the empty
         * generator after the cache, this would have to be possible to disable
         * to work in sync operation with backend.
         */
        return get_tile (tile_store, x, y, z);
      case GEGL_TILE_IS_CACHED:
        return (gpointer)gegl_tile_handler_cache_has_tile (cache, x, y, z);
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
            return (gpointer)action;
          break;
        }
      case GEGL_TILE_REFETCH:
        gegl_tile_handler_cache_invalidate (cache, x, y, z);
        break;
      case GEGL_TILE_VOID:
        gegl_tile_handler_cache_void (cache, x, y, z);
        break;
      default:
        break;
    }

  return gegl_tile_handler_chain_up (handler, command, x, y, z, data);
}

static void
gegl_tile_handler_cache_class_init (GeglTileHandlerCacheClass *class)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (class);
  GeglTileSourceClass *source_class  = GEGL_TILE_SOURCE_CLASS (class);

  gobject_class->finalize = finalize;
  gobject_class->dispose  = dispose;
  source_class->command   = gegl_tile_handler_cache_command;
}

static void
gegl_tile_handler_cache_init (GeglTileHandlerCache *cache)
{
  gegl_tile_cache_init ();
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
      CacheItem *item = link->data;
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

/* returns the requested Tile if it is in the cache, NULL otherwize.
 */
static GeglTile *
gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  CacheItem *result;
  CacheItem pin;

  pin.x = x;
  pin.y = y;
  pin.z = z;
  pin.handler = cache;

#ifdef ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  result = g_hash_table_lookup (cache_ht, &pin);
  if (result)
    {
      g_queue_remove (cache_queue, result);
      g_queue_push_head (cache_queue, result);
#ifdef ENABLE_MT
      g_static_mutex_unlock (&mutex);
#endif
      return gegl_tile_ref (result->tile);
    }
#ifdef ENABLE_MT
  g_static_mutex_unlock (&mutex);
#endif
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

static gboolean
gegl_tile_handler_cache_trim (GeglTileHandlerCache *cache)
{
  CacheItem *last_writable;
 
#ifdef ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  last_writable = g_queue_pop_tail (cache_queue);

  if (last_writable != NULL)
    {
      g_hash_table_remove (cache_ht, last_writable);
      cache_total  -= last_writable->tile->size;
      gegl_tile_unref (last_writable->tile);
      g_slice_free (CacheItem, last_writable);
#ifdef ENABLE_MT
      g_static_mutex_unlock (&mutex);
#endif
      return TRUE;
    }
#ifdef ENABLE_MT
      g_static_mutex_unlock (&mutex);
#endif

  return FALSE;
}

static void
gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                    gint                  x,
                                    gint                  y,
                                    gint                  z)
{
  GList *link;

#ifdef ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      if (tile != NULL &&
          item->x == x &&
          item->y == y &&
          item->z == z &&
          item->handler == cache)
        {
          cache_total  -= item->tile->size;
          tile->tile_storage = NULL;
          tile->stored_rev = tile->rev; /* to cheat it out of being stored */
          gegl_tile_unref (tile);
          g_hash_table_remove (cache_ht, item);
          g_slice_free (CacheItem, item);
          g_queue_delete_link (cache_queue, link);
#ifdef ENABLE_MT
          g_static_mutex_unlock (&mutex);
#endif
          return;
        }
    }
#ifdef ENABLE_MT
  g_static_mutex_unlock (&mutex);
#endif
}


static void
gegl_tile_handler_cache_void (GeglTileHandlerCache *cache,
                              gint                  x,
                              gint                  y,
                              gint                  z)
{
  GList *link;

  if (!cache_queue)
    return;

#ifdef ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      if (tile != NULL &&
          item->x == x &&
          item->y == y &&
          item->z == z &&
          item->handler == cache)
        {
          gegl_tile_void (tile);
          cache_total -= item->tile->size;
          gegl_tile_unref (tile);
          g_hash_table_remove (cache_ht, item);
          g_slice_free (CacheItem, item);
          g_queue_delete_link (cache_queue, link);
#ifdef ENABLE_MT
          g_static_mutex_unlock (&mutex);
#endif
          return;
        }
    }
#ifdef ENABLE_MT
  g_static_mutex_unlock (&mutex);
#endif
}

void
gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                GeglTile             *tile,
                                gint                  x,
                                gint                  y,
                                gint                  z)
{
  CacheItem *item = g_slice_new (CacheItem);
  guint      count;

  item->handler = cache;
  item->tile    = gegl_tile_ref (tile);
  item->x       = x;
  item->y       = y;
  item->z       = z;

#if ENABLE_MT
  g_static_mutex_lock (&mutex);
#endif
  cache_total  += item->tile->size;
  g_queue_push_head (cache_queue, item);

  count = g_queue_get_length (cache_queue);
  g_hash_table_insert (cache_ht, item, item);

  while (cache_total > gegl_config()->cache_size)
    {
      /*GEGL_NOTE(GEGL_DEBUG_CACHE, "cache_total:%i > cache_size:%i", cache_total, gegl_config()->cache_size);
      GEGL_NOTE(GEGL_DEBUG_CACHE, "%f%% hit:%i miss:%i  %i]", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses, g_queue_get_length (cache_queue));*/
      gegl_tile_handler_cache_trim (cache);
    }
#if ENABLE_MT
  g_static_mutex_unlock (&mutex);
#endif
}
