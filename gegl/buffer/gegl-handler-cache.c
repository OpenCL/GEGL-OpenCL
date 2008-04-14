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

#include "../gegl-types.h"
#include "gegl-buffer.h"
#include "gegl-tile.h"
#include "gegl-handler-cache.h"


static gboolean    gegl_handler_cache_wash     (GeglHandlerCache *cache);

static GeglTile *  gegl_handler_cache_get_tile (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);

static gboolean    gegl_handler_cache_has_tile (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);
static void        gegl_handler_cache_insert   (GeglHandlerCache *cache,
                                                GeglTile         *tile,
                                                gint              x,
                                                gint              y,
                                                gint              z);
static void        gegl_handler_cache_void     (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);

G_DEFINE_TYPE (GeglHandlerCache, gegl_handler_cache, GEGL_TYPE_HANDLER)

enum
{
  PROP_0,
  PROP_SIZE,
  PROP_WASH_PERCENTAGE
};

typedef struct CacheItem
{
  GeglTile *tile;
  gint      x;
  gint      y;
  gint      z;
} CacheItem;

static void
finalize (GObject *object)
{
  GeglHandlerCache *cache = (GeglHandlerCache *) object;

  g_queue_free (cache->queue);

  G_OBJECT_CLASS (gegl_handler_cache_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
  GeglHandlerCache *cache = (GeglHandlerCache *) object;
  CacheItem        *item;

  if (0)
    g_printerr ("Disposing tile-cache of size %i, hits: %i misses: %i  hit percentage:%f)\n",
                cache->size, cache->hits, cache->misses,
                cache->hits * 100.0 / (cache->hits + cache->misses));

  while ((item = g_queue_pop_head (cache->queue)))
    {
      g_object_unref (item->tile);
      g_slice_free (CacheItem, item);
    }

  G_OBJECT_CLASS (gegl_handler_cache_parent_class)->dispose (object);
}

static GeglTile *
get_tile (GeglSource *tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglHandlerCache *cache    = GEGL_HANDLER_CACHE (tile_store);
  GeglSource     *source = GEGL_HANDLER (tile_store)->source;
  GeglTile         *tile     = NULL;

  tile = gegl_handler_cache_get_tile (cache, x, y, z);
  if (tile)
    {
      cache->hits++;
      return tile;
    }
  cache->misses++;

  if (source)
    tile = gegl_source_get_tile (source, x, y, z);

  if (tile)
    gegl_handler_cache_insert (cache, tile, x, y, z);

  return tile;
}


static gpointer
command (GeglSource    *tile_store,
         GeglTileCommand  command,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  GeglHandler      *handler = GEGL_HANDLER (tile_store);
  GeglHandlerCache *cache   = GEGL_HANDLER_CACHE (handler);

  /* FIXME: replace with switch */

  if (command == GEGL_TILE_GET)
    {
      return get_tile (tile_store, x, y, z);
    }
  
  if (command == GEGL_TILE_IS_CACHED)
    {
      return (gpointer)gegl_handler_cache_has_tile (cache, x, y, z);
    }
  if (command == GEGL_TILE_EXIST)
    {
      gboolean is_cached = gegl_handler_cache_has_tile (cache, x, y, z);
      if (is_cached)
        return (gpointer)TRUE; /* XXX: perhaps we could return an integer/pointer
                      * value over the bus instead of a boolean?
                      */
      /* otherwise pass on the request */
    }

  if (command == GEGL_TILE_IDLE)
    {
      gboolean action = gegl_handler_cache_wash (cache);
      if (action)
        return (gpointer)action;
    }
  if (command == GEGL_TILE_VOID)
    {
      gegl_handler_cache_void (cache, x, y, z);
      return NULL;
    }
  return gegl_handler_chain_up (handler, command, x, y, z, data);
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglHandlerCache *cache = GEGL_HANDLER_CACHE (gobject);

  switch (property_id)
    {
      case PROP_SIZE:
        g_value_set_int (value, cache->size);
        break;

      case PROP_WASH_PERCENTAGE:
        g_value_set_int (value, cache->wash_percentage);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglHandlerCache *cache = GEGL_HANDLER_CACHE (gobject);

  switch (property_id)
    {
      case PROP_SIZE:
        cache->size = g_value_get_int (value);
        return;

      case PROP_WASH_PERCENTAGE:
        cache->wash_percentage = g_value_get_int (value);
        return;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_handler_cache_class_init (GeglHandlerCacheClass *class)
{
  GObjectClass      *gobject_class  = G_OBJECT_CLASS (class);
  GeglSourceClass *source_class = GEGL_SOURCE_CLASS (class);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize     = finalize;
  gobject_class->dispose      = dispose;

  source_class->command  = command;

  g_object_class_install_property (gobject_class, PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     "size",
                                                     "Number of tiles in cache",
                                                     0, G_MAXINT, 32,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_WASH_PERCENTAGE,
                                   g_param_spec_int ("wash-percentage",
                                                     "wash percentage",
                                                     "(integer 0..100, percentage to wash)",
                                                     0, 100, 20,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gegl_handler_cache_init (GeglHandlerCache *cache)
{
  cache->queue = g_queue_new ();
}


/* write the least recently used dirty tile to disk if it
 * is in the wash_percentage (20%) least recently used tiles,
 * calling this function in an idle handler distributes the
 * tile flushing overhead over time.
 */
gboolean
gegl_handler_cache_wash (GeglHandlerCache *cache)
{
  GeglTile  *last_dirty = NULL;
  guint      count      = 0;
  gint       wash_tiles = cache->wash_percentage * cache->size / 100;
  GList     *link;

  for (link = g_queue_peek_head_link (cache->queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      count++;
      if (!gegl_tile_is_stored (tile))
        {
          if (count > cache->size - wash_tiles)
            {
              last_dirty = tile;
            }
        }
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
gegl_handler_cache_get_tile (GeglHandlerCache *cache,
                             gint              x,
                             gint              y,
                             gint              z)
{
  GList *link;

  for (link = g_queue_peek_head_link (cache->queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      if (tile != NULL &&
          item->x == x &&
          item->y == y &&
          item->z == z)
        {
          /* move the link to the front of the queue */
          if (link->prev != NULL)
            {
              g_queue_unlink (cache->queue, link);
              g_queue_push_head_link (cache->queue, link);
            }

          return g_object_ref (tile);
        }
    }

  return NULL;
}


gboolean
gegl_handler_cache_has_tile (GeglHandlerCache *cache,
                             gint           x,
                             gint           y,
                             gint           z)
{
  GeglTile *tile = gegl_handler_cache_get_tile (cache, x, y, z);

  if (tile)
    {
      g_object_unref (G_OBJECT (tile));
      return TRUE;
    }

  return FALSE;
}

static gboolean
gegl_handler_cache_trim (GeglHandlerCache *cache)
{
  CacheItem *last_writable = g_queue_pop_tail (cache->queue);

  if (last_writable != NULL)
    {
      g_object_unref (last_writable->tile);
      g_slice_free (CacheItem, last_writable);
      return TRUE;
    }

  return FALSE;
}


static void
gegl_handler_cache_void (GeglHandlerCache *cache,
                         gint              x,
                         gint              y,
                         gint              z)
{
  GList *link;

  for (link = g_queue_peek_head_link (cache->queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      if (tile != NULL &&
          item->x == x &&
          item->y == y &&
          item->z == z)
        {
          gegl_tile_void (tile);
          g_object_unref (tile);
          g_slice_free (CacheItem, item);
          g_queue_delete_link (cache->queue, link);
          return;
        }
    }
}

void
gegl_handler_cache_insert (GeglHandlerCache *cache,
                           GeglTile         *tile,
                           gint              x,
                           gint              y,
                           gint              z)
{
  CacheItem *item = g_slice_new (CacheItem);
  guint      count;

  item->tile = g_object_ref (tile);
  item->x    = x;
  item->y    = y;
  item->z    = z;

  g_queue_push_head (cache->queue, item);

  count = g_queue_get_length (cache->queue);

  if (count > cache->size)
    {
      gint to_remove = count - cache->size;

      while (--to_remove && gegl_handler_cache_trim (cache)) ;
    }
}
