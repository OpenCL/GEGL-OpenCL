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

#include "gegl-config.h"
#include "../gegl-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-debug.h"

static GQueue *cache_queue = NULL;
static gint    cache_wash_percentage = 20;
static gint    cache_hits = 0;
static gint    cache_misses = 0;

static gint    cache_total = 0;  /* approximate amount of bytes stored */
static gint    clones_ones = 0;  /* approximate amount of bytes stored */

struct _GeglTileHandlerCache
{
  GeglTileHandler parent_instance;
  GSList *free_list;
};

void gegl_tile_cache_init (void)
{
  if (cache_queue == NULL)
    cache_queue = g_queue_new ();
}

void gegl_tile_cache_destroy (void)
{
  if (cache_queue)
    g_queue_free (cache_queue);
  cache_queue = NULL;
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

G_DEFINE_TYPE (GeglTileHandlerCache, gegl_tile_handler_cache, GEGL_TYPE_TILE_HANDLER)

typedef struct CacheItem
{ 
  GeglTileHandlerCache *handler; /* The specific handler that cached this item*/
  GeglTile *tile;                /* The tile */

  gint      x;                   /* The coordinates this tile was cached for */
  gint      y;
  gint      z;
} CacheItem;

static void
finalize (GObject *object)
{
  GeglTileHandlerCache *cache;
  cache = (GeglTileHandlerCache *) object;

  G_OBJECT_CLASS (gegl_tile_handler_cache_parent_class)->finalize (object);
}

static void
queue_each (gpointer itm,
            gpointer userdata)
{
  CacheItem *item = itm;
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

  cache = GEGL_TILE_HANDLER_CACHE (object);

  /* only throw out items belonging to this cache instance */

  cache->free_list = NULL;
  g_queue_foreach (cache_queue, queue_each, cache);
  for (iter = cache->free_list; iter; iter = g_slist_next (iter))
    {
        item = iter->data;
        if (item->tile)
          {
            cache_total -= item->tile->size;
            clones_ones = 0; /* XXX */
            g_object_unref (item->tile);
          }
        g_queue_remove (cache_queue, item);
        g_slice_free (CacheItem, item);
    }
  g_slist_free (cache->free_list);
  cache->free_list = NULL;

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
      cache_hits++;
      return tile;
    }
  cache_misses++;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);

  if (tile)
    gegl_tile_handler_cache_insert (cache, tile, x, y, z);

  return tile;
}

static gpointer
command (GeglTileSource  *tile_store,
         GeglTileCommand  command,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  GeglTileHandler      *handler = GEGL_HANDLER (tile_store);
  GeglTileHandlerCache *cache   = GEGL_TILE_HANDLER_CACHE (handler);

  /* FIXME: replace with switch */
  switch (command)
    {
      case GEGL_TILE_SET:
        /* nothing to do */
        break; /* chain up */
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
        break; /* chain up */
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
        break; /* chain up */
      case GEGL_TILE_IDLE:
        {
          gboolean action = gegl_tile_handler_cache_wash (cache);
          if (action)
            return (gpointer)action;
          break;
        }
      case GEGL_TILE_VOID:
        gegl_tile_handler_cache_void (cache, x, y, z);
        /*if (z!=0)
          return (void*)0xdead700;*/
        /* fallthrough */
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
  source_class->command   = command;
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
GeglTile *
gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  GList *link;

  for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      if (tile != NULL &&
          item->handler == cache &&
          item->x == x &&
          item->y == y &&
          item->z == z)
        {
          /* move the link to the front of the queue */
          if (link->prev != NULL)
            {
              g_queue_unlink (cache_queue, link);
              g_queue_push_head_link (cache_queue, link);
            }

          return g_object_ref (tile);
        }
    }

  return NULL;
}


gboolean
gegl_tile_handler_cache_has_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  GeglTile *tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);

  if (tile)
    {
      g_object_unref (G_OBJECT (tile));
      return TRUE;
    }

  return FALSE;
}

static gboolean
gegl_tile_handler_cache_trim (GeglTileHandlerCache *cache)
{
  CacheItem *last_writable = g_queue_pop_tail (cache_queue);

  if (last_writable != NULL)
    {
      cache_total  -= last_writable->tile->size;
      g_object_unref (last_writable->tile);
      g_slice_free (CacheItem, last_writable);
      return TRUE;
    }

  return FALSE;
}


static void
gegl_tile_handler_cache_void (GeglTileHandlerCache *cache,
                              gint                  x,
                              gint                  y,
                              gint                  z)
{
  GList *link;

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
          cache_total  -= item->tile->size;
          g_object_unref (tile);
          g_slice_free (CacheItem, item);
          g_queue_delete_link (cache_queue, link);
          return;
        }
    }
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
  item->tile    = g_object_ref (tile);
  item->x       = x;
  item->y       = y;
  item->z       = z;
  cache_total  += item->tile->size;

  g_queue_push_head (cache_queue, item);

  count = g_queue_get_length (cache_queue);

  while (cache_total > gegl_config()->cache_size)
    {
      /*GEGL_NOTE(CACHE, "cache_total:%i > cache_size:%i", cache_total, gegl_config()->cache_size);
      GEGL_NOTE(CACHE, "%f%% hit:%i miss:%i  %i]", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses, g_queue_get_length (cache_queue));*/
      gegl_tile_handler_cache_trim (cache);
    }
}
