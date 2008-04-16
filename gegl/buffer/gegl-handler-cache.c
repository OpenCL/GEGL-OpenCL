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
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-handler-cache.h"

/* FIXME: this global cache should have configurable size and wash percentage */

static GQueue *cache_queue = NULL;
static gint    cache_size = 512;
static gint    cache_wash_percentage = 20;
static gint    cache_hits = 0;
static gint    cache_misses = 0;

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

static gboolean    gegl_handler_cache_wash     (GeglHandlerCache *cache);

static GeglTile *  gegl_handler_cache_get_tile (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);

static gboolean    gegl_handler_cache_has_tile (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);
       void        gegl_handler_cache_insert   (GeglHandlerCache *cache,
                                                GeglTile         *tile,
                                                gint              x,
                                                gint              y,
                                                gint              z);
static void        gegl_handler_cache_void     (GeglHandlerCache *cache,
                                                gint              x,
                                                gint              y,
                                                gint              z);

G_DEFINE_TYPE (GeglHandlerCache, gegl_handler_cache, GEGL_TYPE_HANDLER)

typedef struct CacheItem
{
  GeglHandlerCache *handler; /* identify the handler as well, thus allowing
                              * all buffers to share a common cache
                              */
  GeglTile *tile;
  gint      x;
  gint      y;
  gint      z;
} CacheItem;


static void
finalize (GObject *object)
{
  GeglHandlerCache *cache;
  cache = (GeglHandlerCache *) object;

  G_OBJECT_CLASS (gegl_handler_cache_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
  GeglHandlerCache *cache;
  CacheItem        *item;
  cache = (GeglHandlerCache *) object;

  if (0)
    g_printerr ("Disposing tile-cache of size %i, hits: %i misses: %i  hit percentage:%f)\n",
                cache_size, cache_hits, cache_misses,
                cache_hits * 100.0 / (cache_hits + cache_misses));

  /* FIXME: only throw out this cache's items */
  while ((item = g_queue_pop_head (cache_queue)))
    {
      g_object_unref (item->tile);
      g_slice_free (CacheItem, item);
    }
  /* FIXME: if queue is empty destroy global queue */

  G_OBJECT_CLASS (gegl_handler_cache_parent_class)->dispose (object);
}

static GeglTile *
get_tile (GeglSource *tile_store,
          gint        x,
          gint        y,
          gint        z)
{
  GeglHandlerCache *cache    = GEGL_HANDLER_CACHE (tile_store);
  GeglSource       *source = GEGL_HANDLER (tile_store)->source;
  GeglTile         *tile     = NULL;

  if(0)g_print ("%f%% hit:%i miss:%i  \r", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses);

  tile = gegl_handler_cache_get_tile (cache, x, y, z);
  if (tile)
    {
      cache_hits++;
      return tile;
    }
  cache_misses++;

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
  switch (command)
    {
      case GEGL_TILE_GET:
        return get_tile (tile_store, x, y, z);
      case GEGL_TILE_IS_CACHED:
        return (gpointer)gegl_handler_cache_has_tile (cache, x, y, z);
      case GEGL_TILE_EXIST:
        {
          gboolean exist = gegl_handler_cache_has_tile (cache, x, y, z);
          if (exist)
            return (gpointer)TRUE;
        }
        break; /* chain up */
      case GEGL_TILE_IDLE:
        {
          gboolean action = gegl_handler_cache_wash (cache);
          if (action)
            return (gpointer)action;
          break;
        }
      case GEGL_TILE_VOID:
        gegl_handler_cache_void (cache, x, y, z);
        /* fallthrough */
      default:
        break;
    }
  return gegl_handler_chain_up (handler, command, x, y, z, data);
}

static void
gegl_handler_cache_class_init (GeglHandlerCacheClass *class)
{
  GObjectClass      *gobject_class  = G_OBJECT_CLASS (class);
  GeglSourceClass *source_class = GEGL_SOURCE_CLASS (class);

  gobject_class->finalize     = finalize;
  gobject_class->dispose      = dispose;

  source_class->command  = command;
}

static void
gegl_handler_cache_init (GeglHandlerCache *cache)
{
  gegl_tile_cache_init ();
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
  gint       wash_tiles = cache_wash_percentage * cache_size / 100;
  GList     *link;

  for (link = g_queue_peek_head_link (cache_queue); link; link = link->next)
    {
      CacheItem *item = link->data;
      GeglTile  *tile = item->tile;

      count++;
      if (!gegl_tile_is_stored (tile))
        {
          if (count > cache_size - wash_tiles)
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
  CacheItem *last_writable = g_queue_pop_tail (cache_queue);

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
          g_object_unref (tile);
          g_slice_free (CacheItem, item);
          g_queue_delete_link (cache_queue, link);
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

  item->handler = cache;
  item->tile    = g_object_ref (tile);
  item->x       = x;
  item->y       = y;
  item->z       = z;

  g_queue_push_head (cache_queue, item);

  count = g_queue_get_length (cache_queue);

  if (count > cache_size)
    {
      gint to_remove = count - cache_size;

      while (--to_remove && gegl_handler_cache_trim (cache)) ;
    }
}
