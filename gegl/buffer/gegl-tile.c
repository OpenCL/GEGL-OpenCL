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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include "string.h" /* memcpy */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-source.h"


G_DEFINE_TYPE (GeglTile, gegl_tile, G_TYPE_OBJECT)
enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_Z,
  PROP_SIZE
};
static GObjectClass *parent_class = NULL;


static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTile *tile = GEGL_TILE (gobject);

  switch (property_id)
    {
      case PROP_X:
        g_value_set_int (value, tile->x);
        break;

      case PROP_Y:
        g_value_set_int (value, tile->y);
        break;

      case PROP_Z:
        g_value_set_int (value, tile->z);
        break;

      case PROP_SIZE:
        g_value_set_int (value, tile->size);
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
  GeglTile *tile = GEGL_TILE (gobject);

  switch (property_id)
    {
      case PROP_X:
        tile->x = g_value_get_int (value);
        return;

      case PROP_Y:
        tile->y = g_value_get_int (value);
        return;

      case PROP_Z:
        tile->z = g_value_get_int (value);
        return;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
  GeglTile *tile = (GeglTile *) object;

  if (!gegl_tile_is_stored (tile))
    gegl_tile_store (tile);

  if (tile->data)
    {
      if (tile->next_shared == tile)
        { /* no clones */
          g_free (tile->data);
          tile->data = NULL;
        }
      else
        {
          tile->prev_shared->next_shared = tile->next_shared;
          tile->next_shared->prev_shared = tile->prev_shared;
        }
    }

#if ENABLE_MP
  if (tile->mutex)
    {
      g_mutex_free (tile->mutex);
      tile->mutex = NULL;
    }
#endif

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
}

static void
gegl_tile_class_init (GeglTileClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->dispose      = dispose;
  parent_class                = g_type_class_peek_parent (class);

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "Horizontal index",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "Vertical index",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Z,
                                   g_param_spec_int ("z", "z", "Pyramid level 0=100% 1=50% 2=25%",
                                                     0, 256, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SIZE,
                                   g_param_spec_int ("size", "size", "size of linear memory buffer in bytes.",
                                                     0, 0, 0,
                                                     G_PARAM_READABLE));
}

static void
gegl_tile_init (GeglTile *tile)
{
  tile->tile_storage    = NULL;
  tile->stored_rev = 0;
  tile->rev        = 0;
  tile->lock       = 0;
  tile->data       = NULL;
  tile->flags      = 0;

  tile->next_shared = tile;
  tile->prev_shared = tile;

#if ENABLE_MP
  tile->mutex = g_mutex_new ();
#endif
}

GeglTile *
gegl_tile_dup (GeglTile *src)
{
  GeglTile *tile = g_object_new (GEGL_TYPE_TILE, NULL);

  tile->rev        = 1;
  tile->stored_rev = 1;
  tile->tile_storage    = src->tile_storage;
  tile->data       = src->data;
  tile->size       = src->size;

  tile->next_shared              = src->next_shared;
  src->next_shared               = tile;
  tile->prev_shared              = src;
  tile->next_shared->prev_shared = tile;

  return tile;
}

GeglTile *
gegl_tile_new (gint size)
{
  GeglTile *tile = g_object_new (GEGL_TYPE_TILE, NULL);

  tile->data       = g_malloc (size);
  tile->size       = size;
  tile->stored_rev = 1;


  return tile;
}

static void
gegl_tile_unclone (GeglTile *tile)
{
  if (tile->next_shared != tile)
    {
      /* the tile data is shared with other tiles,
       * create a local copy
       */
      tile->data                     = g_memdup (tile->data, tile->size);
      tile->prev_shared->next_shared = tile->next_shared;
      tile->next_shared->prev_shared = tile->prev_shared;
      tile->prev_shared              = tile;
      tile->next_shared              = tile;
    }
}

static gint total_locks   = 0;
static gint total_unlocks = 0;

void
gegl_tile_lock (GeglTile *tile)
{
  if (tile->lock != 0)
    {
      g_warning ("locking a tile for the second time");
    }
  total_locks++;

#if ENABLE_MP
  g_mutex_lock (tile->mutex);
#endif

  tile->lock++;
  /*fprintf (stderr, "global tile locking: %i %i\n", locks, unlocks);*/

  gegl_tile_unclone (tile);
  /*gegl_buffer_add_dirty (tile->buffer, tile->x, tile->y);*/
}

static void
gegl_tile_void_pyramid (GeglTile *tile)
{
  /* should, to tile->tile_storage, request it's toplevel tile, and mark
   * it as dirty, to force a recomputation of it's toplevel at the
   * next subdivision request. NB: a full voiding might not be neccesary,
   * forcing a rerender of just the dirtied part might be better, more
   * similar to how it was done in horizon, this will only work with 4->1 px
   * averageing.
   */
  gint x, y, z;

  x = tile->x;
  y = tile->y;
  z = 0;/*tile->z;*/

  for (z = 1; z < 10; z++)
    {
#if 0
      gint ver = (y % 2);
      gint hor = (x % 2);
#endif
      x /= 2;
      y /= 2;

      gegl_tile_source_void (GEGL_TILE_SOURCE (tile->tile_storage), x, y, z);
#if 0
      /* FIXME: reenable this code */
      if (!ver)
        {
          if (!hor)
            {
              gegl_tile_source_void_tl (GEGL_TILE_SOURCE (tile->tile_storage), x,y,z);
            }
          else
            {
              gegl_tile_source_void_tr (GEGL_TILE_SOURCE (tile->tile_storage), x,y,z);              
            }
        }
      else
        {
          if (!hor)
            {
              gegl_tile_source_void_bl (GEGL_TILE_SOURCE (tile->tile_storage), x,y,z);              
            }
          else
            {
			  gegl_tile_source_void_br (GEGL_TILE_SOURCE (tile->tile_storage), x,y,z);
            }
        }
#endif
    }
}

void
gegl_tile_unlock (GeglTile *tile)
{
  total_unlocks++;
  if (tile->lock == 0)
    {
      g_warning ("unlocked a tile with lock count == 0");
    }
  tile->lock--;
  if (tile->lock == 0 &&
      tile->z == 0)
    {
      gegl_tile_void_pyramid (tile);
    }
  if (tile->lock==0)
    tile->rev++;
#if ENABLE_MP
  g_mutex_unlock (tile->mutex);
#endif
}


gboolean
gegl_tile_is_stored (GeglTile *tile)
{
  return tile->stored_rev == tile->rev;
}

void
gegl_tile_void (GeglTile *tile)
{
  tile->stored_rev = tile->rev;
  tile->tile_storage = NULL;
  /* FIXME: make sure the tile is evicted from any tile_storage/buffer caches
   * as well
   */
}

void
gegl_tile_cpy (GeglTile *src,
               GeglTile *dst)
{
  gegl_tile_lock (dst);

  g_free (dst->data);
  dst->data = NULL;

  dst->next_shared              = src->next_shared;
  src->next_shared              = dst;
  dst->prev_shared              = src;
  dst->next_shared->prev_shared = dst;

  dst->data = src->data;

  gegl_tile_unlock (dst);
}

void
gegl_tile_swp (GeglTile *a,
               GeglTile *b)
{
  guchar *tmp;

  gegl_tile_unclone (a);
  gegl_tile_unclone (b);

/*  gegl_buffer_add_dirty (a->buffer, a->x, a->y);
   gegl_buffer_add_dirty (b->buffer, b->x, b->y);*/

  g_assert (a->size == b->size);

  tmp     = a->data;
  a->data = b->data;
  b->data = tmp;
}

unsigned char *
gegl_tile_get_data (GeglTile *tile)
{
  return tile->data;
}

gboolean gegl_tile_store (GeglTile *tile)
{
  if (gegl_tile_is_stored (tile))
    return TRUE;
  if (tile->tile_storage == NULL)
    return FALSE;
  return gegl_tile_source_set_tile (GEGL_TILE_SOURCE (tile->tile_storage),
                                    tile->x,
                                    tile->y,
                                    tile->z, tile);
}

/* compute the tile indice of a coordinate
 * the stride is the width/height of tiles along the axis of coordinate
 */
gint
gegl_tile_indice (gint coordinate,
                  gint stride)
{
  if (coordinate >= 0)
    return coordinate / stride;
  return (((coordinate + 1) / stride) - 1);
}

/* computes the positive integer remainder (also for negative dividends)
 */
#define REMAINDER(dividend, divisor) \
                   (((dividend) < 0) ? \
                    (divisor) - 1 - ((-((dividend) + 1)) % (divisor)) : \
                    (dividend) % (divisor))


/* compute the offset into the containing tile a coordinate has,
 * the stride is the width/height of tiles along the axis of coordinate
 */
gint
gegl_tile_offset (gint coordinate,
                  gint stride)
{
  return REMAINDER (coordinate, stride);
}
