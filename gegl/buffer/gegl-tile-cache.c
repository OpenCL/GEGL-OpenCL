/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <stdio.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "../gegl-types.h"
#include "gegl-buffer.h"
#include "gegl-tile.h"
#include "gegl-tile-cache.h"

GeglTileCache *gegl_tile_cache_new (void);


static
gboolean    gegl_tile_cache_wash (GeglTileCache *cache);

static
GeglTile *gegl_tile_cache_get_tile (GeglTileCache *cache,
                                    gint           x,
                                    gint           y,
                                    gint           z);

static
gboolean    gegl_tile_cache_has_tile (GeglTileCache *cache,
                                      gint           x,
                                      gint           y,
                                      gint           z);
static
void        gegl_tile_cache_insert (GeglTileCache *cache,
                                    GeglTile      *tile,
                                    gint           x,
                                    gint           y,
                                    gint           z);

G_DEFINE_TYPE (GeglTileCache, gegl_tile_cache, GEGL_TYPE_TILE_TRAIT)

enum
{
  PROP_0,
  PROP_SIZE,
  PROP_WASH_PERCENTAGE
};
static GObjectClass *parent_class = NULL;

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
  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}
static void
dispose (GObject *object)
{
  GeglTileCache *cache;
  GSList        *list;
  guint          count;

  cache = (GeglTileCache *) object;
  count = g_slist_length (cache->list);

  if (0) fprintf (stderr, "Disposing tile-cache of size %i, hits: %i misses: %i  hit percentage:%f)\n", cache->size, cache->hits, cache->misses,
                  cache->hits * 100.0 / (cache->hits + cache->misses));

  while ((list = cache->list))
    {
      CacheItem *item = list->data;
      g_object_unref (item->tile);
      cache->list = g_slist_remove (cache->list, item);
      g_free (item);
    }

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
}

static GeglTile *
get_tile (GeglTileStore *tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglTileCache *cache  = GEGL_TILE_CACHE (tile_store);
  GeglTileStore *source = GEGL_TILE_TRAIT (tile_store)->source;
  GeglTile      *tile   = NULL;

  tile = gegl_tile_cache_get_tile (cache, x, y, z);
  if (tile)
    {
      cache->hits++;

      return tile;
    }
  cache->misses++;

  if (source)
    tile = gegl_tile_store_get_tile (source, x, y, z);

  if (tile)
    {
      gegl_tile_cache_insert (cache, tile, x, y, z);
    }
  return tile;
}


static gboolean
gegl_tile_cache_void (GeglTileCache *cache, gint x, gint y, gint z);

static gboolean
message (GeglTileStore  *tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglTileTrait *trait = GEGL_TILE_TRAIT (tile_store);
  GeglTileCache *cache = GEGL_TILE_CACHE (trait);

  if (message == GEGL_TILE_IS_CACHED)
    {
      return gegl_tile_cache_has_tile (cache, x, y, z);
    }
  if (message == GEGL_TILE_EXIST)
    {
      gboolean is_cached = gegl_tile_cache_has_tile (cache, x, y, z);
      if (is_cached)
        return TRUE;
      /* otherwise pass on the request */
    }

  if (message == GEGL_TILE_IDLE)
    {
      gboolean action = gegl_tile_cache_wash (cache);
      if (action)
        return action;
    }
  if (message == GEGL_TILE_VOID)
    {
      gegl_tile_cache_void (cache, x, y, z);
    }
  if (trait->source)
    return gegl_tile_store_message (trait->source, message, x, y, z, data);
  return FALSE;
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileCache *cache = GEGL_TILE_CACHE (gobject);

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
  GeglTileCache *cache = GEGL_TILE_CACHE (gobject);

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
gegl_tile_cache_class_init (GeglTileCacheClass *class)
{
  GObjectClass       *gobject_class    = G_OBJECT_CLASS (class);
  GeglTileStoreClass *tile_store_class = GEGL_TILE_STORE_CLASS (class);

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  gobject_class->finalize = finalize;
  gobject_class->dispose  = dispose;

  g_object_class_install_property (gobject_class, PROP_SIZE,
                                   g_param_spec_int ("size", "size", "Number of tiles in cache",
                                                     0, G_MAXINT, 32,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_WASH_PERCENTAGE,
                                   g_param_spec_int ("wash-percentage", "wash percentage", "(integer 0..100, percentage to wash)",
                                                     0, 100, 20,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));



  tile_store_class->get_tile = get_tile;
  tile_store_class->message  = message;
}

static void
gegl_tile_cache_init (GeglTileCache *cache)
{
  cache->list = NULL;
}


/* create a new tile cache, associated with the given gegl_buffer and capable of holding size tiles */
GeglTileCache *
gegl_tile_cache_new (void)
{
  return g_object_new (GEGL_TYPE_TILE_CACHE, NULL);
}

/* write the least recently used dirty tile to disk if it
 * is in the wash_percentage (20%) least recently used tiles,
 * calling this function in an idle handler distributes the
 * tile flushing overhead over time.
 */
gboolean
gegl_tile_cache_wash (GeglTileCache *cache)
{
  GeglTile *last_dirty = NULL;
  guint     count      = 0;

  gint      wash_tiles = cache->wash_percentage * cache->size / 100;

  GSList   *list = cache->list;

  while (list)
    {
      CacheItem *item = list->data;
      GeglTile  *tile = item->tile;

      count++;
      if (!gegl_tile_is_stored (tile))
        {
          if (count > cache->size - wash_tiles)
            {
              last_dirty = tile;
            }
        }
      list = list->next;
    }
  if (last_dirty != NULL)
    {
      gegl_tile_store (last_dirty);
      return TRUE;
    }
  return FALSE;
}

/* returns the requested Tile * if it is in the cache NULL
 * otherwize.
 */
GeglTile *gegl_tile_cache_get_tile (GeglTileCache *cache,
                                    gint           x,
                                    gint           y,
                                    gint           z)
{
  GeglTile *tile = NULL;

  if (cache->size > 0)
    {
      GSList *list = cache->list;
      GSList *prev = NULL;

      while (list)
        {
          CacheItem *cur_item = list->data;
          GeglTile  *cur_tile = cur_item->tile;

          if (cur_tile != NULL &&
              cur_item->x == x &&
              cur_item->y == y &&
              cur_item->z == z)
            {
              tile = cur_tile;
              break;
            }
          prev = list;
          list = list->next;
        }

      if (tile != NULL)
        {
          g_object_ref (G_OBJECT (tile));

          /* reorder list */
          if (prev)
            {
              if (prev->next)
                {
                  prev->next = list->next;
                }
              if (cache->list)
                list->next = cache->list;
              cache->list = list;
            }
        }
      else
        {
        }
    }
  return tile;
}


gboolean
gegl_tile_cache_has_tile (GeglTileCache *cache,
                          gint           x,
                          gint           y,
                          gint           z)
{
  GeglTile *tile = gegl_tile_cache_get_tile (cache, x, y, z);

  if (tile)
    {
      g_object_unref (G_OBJECT (tile));
      return TRUE;
    }
  else
    {
    }
  return FALSE;
}

static gboolean
gegl_tile_cache_trim (GeglTileCache *cache)
{
  CacheItem *last_writable = NULL;

  if (!cache->list)
    return FALSE;

  last_writable = g_slist_last (cache->list)->data;

  if (last_writable != NULL)
    {
      GeglTile *tile = last_writable->tile;

      g_object_unref (tile);
      cache->list = g_slist_remove (cache->list, last_writable);
      g_free (last_writable);
      return TRUE;
    }
  return FALSE;
}


static gboolean
gegl_tile_cache_void (GeglTileCache *cache, gint x, gint y, gint z)
{
  CacheItem *item = NULL;

  GeglTile  *tile = NULL;

  if (cache->size > 0)
    {
      GSList *list = cache->list;
      GSList *prev = NULL;

      while (list)
        {
          CacheItem *cur_item = list->data;
          GeglTile  *cur_tile = cur_item->tile;

          if (cur_tile != NULL &&
              cur_item->x == x &&
              cur_item->y == y &&
              cur_item->z == z)
            {
              tile = cur_tile;
              item = cur_item;
              break;
            }
          prev = list;
          list = list->next;
        }

      if (tile != NULL)
        {
          /* reorder list placing ourselves at start to make removal faster */
          if (prev)
            {
              if (prev->next)
                {
                  prev->next = list->next;
                }
              if (cache->list)
                list->next = cache->list;
              cache->list = list;
            }

          gegl_tile_void (tile);
          g_object_unref (tile);
          cache->list = g_slist_remove (cache->list, item);
          g_free (item);
        }
      else
        {
        }
    }
  return FALSE;
}

void
gegl_tile_cache_insert (GeglTileCache *cache,
                        GeglTile      *tile,
                        gint           x,
                        gint           y,
                        gint           z)
{
  guint      count;
  CacheItem *item = g_malloc (sizeof (CacheItem));

  g_object_ref (G_OBJECT (tile));
  item->tile = tile;
  item->x    = x;
  item->y    = y;
  item->z    = z;

  cache->list = g_slist_prepend (cache->list, item);
  count       = g_slist_length (cache->list);

  if (count > cache->size)
    {
      gint to_remove = count - cache->size;

      while (--to_remove && gegl_tile_cache_trim (cache)) ;
    }
}
