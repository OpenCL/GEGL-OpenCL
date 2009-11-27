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
 * Copyright 2006-2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include "string.h" /* memcpy */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-source.h"
#include "gegl-tile-storage.h"

#include "gegl-utils.h"

static void default_free (gpointer data,
                          gpointer userdata)
{
  gegl_free (data);
}

GeglTile *gegl_tile_ref (GeglTile *tile)
{
  g_atomic_int_inc (&tile->ref_count);
  return tile;
}

void gegl_tile_unref (GeglTile *tile)
{
  if (!g_atomic_int_dec_and_test (&tile->ref_count))
    return;

  if (!gegl_tile_is_stored (tile))
    gegl_tile_store (tile);

  if (tile->data)
    {
      if (tile->next_shared == tile)
        { /* no clones */
          if (tile->destroy_notify)
            tile->destroy_notify (tile->data, tile->destroy_notify_data);
          tile->data = NULL;
        }
      else
        {
          tile->prev_shared->next_shared = tile->next_shared;
          tile->next_shared->prev_shared = tile->prev_shared;
        }
    }

#if ENABLE_MT
  if (tile->mutex)
    {
      g_mutex_free (tile->mutex);
      tile->mutex = NULL;
    }
#endif
  g_slice_free (GeglTile, tile);
}

GeglTile *
gegl_tile_new_bare (void)
{
  GeglTile *tile = g_slice_new0 (GeglTile);
  tile->ref_count = 1;
  tile->tile_storage = NULL;
  tile->stored_rev = 0;
  tile->rev        = 0;
  tile->lock       = 0;
  tile->data       = NULL;

  tile->next_shared = tile;
  tile->prev_shared = tile;

#if ENABLE_MT
  tile->mutex = g_mutex_new ();
#endif
  tile->destroy_notify = default_free;

  return tile;
}

GeglTile *
gegl_tile_dup (GeglTile *src)
{
  GeglTile *tile = gegl_tile_new_bare ();

  tile->rev        = 1;
  tile->stored_rev = 1;
  tile->tile_storage    = src->tile_storage;
  tile->data       = src->data;
  tile->size       = src->size;

  tile->next_shared              = src->next_shared;
  src->next_shared               = tile;
  tile->prev_shared              = src;
#if ENABLE_MT
  if (tile->next_shared != src)
    {
      g_mutex_lock (tile->next_shared->mutex);
    }
#endif
  tile->next_shared->prev_shared = tile;
#if ENABLE_MT
  if (tile->next_shared != src)
    {
      g_mutex_unlock (tile->next_shared->mutex);
    }
#endif

  return tile;
}

GeglTile *
gegl_tile_new (gint size)
{
  GeglTile *tile = gegl_tile_new_bare ();

  tile->data       = gegl_malloc (size);
  tile->size       = size;
  tile->stored_rev = 1;

  return tile;
}

static gpointer
gegl_memdup (gpointer src, gsize size)
{
  gpointer ret;
  ret = gegl_malloc (size);
  memcpy (ret, src, size);
  return ret;
}

static void
gegl_tile_unclone (GeglTile *tile)
{
  if (tile->next_shared != tile)
    {
      /* the tile data is shared with other tiles,
       * create a local copy
       */
      tile->data                     = gegl_memdup (tile->data, tile->size);
      tile->prev_shared->next_shared = tile->next_shared;
      tile->next_shared->prev_shared = tile->prev_shared;
      tile->prev_shared              = tile;
      tile->next_shared              = tile;
    }
}
#if 0
static gint total_locks   = 0;
static gint total_unlocks = 0;
#endif

void gegl_bt (void);

void
gegl_tile_lock (GeglTile *tile)
{
#if ENABLE_MT
  g_mutex_lock (tile->mutex);
#endif

  if (tile->lock != 0)
    {
      g_warning ("strange tile lock count: %i", tile->lock);
      gegl_bt ();
    }
#if 0
  total_locks++;
#endif

  tile->lock++;
  /*fprintf (stderr, "global tile locking: %i %i\n", locks, unlocks);*/

  gegl_tile_unclone (tile);
}

static void
_gegl_tile_void_pyramid (GeglTileSource *source,
                         gint            x,
                         gint            y,
                         gint            z)
{
  if (z > ((GeglTileStorage*)source)->seen_zoom)
    return;
  gegl_tile_source_void (source, x, y, z);
  _gegl_tile_void_pyramid (source, x/2, y/2, z+1);
}

static void
gegl_tile_void_pyramid (GeglTile *tile)
{
  if (tile->tile_storage && 
      tile->tile_storage->seen_zoom &&
      tile->z == 0) /* we only accepting voiding the base level */
    {
      _gegl_tile_void_pyramid (GEGL_TILE_SOURCE (tile->tile_storage), 
                               tile->x/2,
                               tile->y/2,
                               tile->z+1);
      return;
    }
}

void
gegl_tile_unlock (GeglTile *tile)
{
#if 0
  total_unlocks++;
#endif
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
#if ENABLE_MT
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
  if (tile->z==0)
    gegl_tile_void_pyramid (tile);
}

void
gegl_tile_cpy (GeglTile *src,
               GeglTile *dst)
{
  gegl_tile_lock (dst);

  gegl_free (dst->data);
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

  g_assert (a->size == b->size);

  tmp     = a->data;
  a->data = b->data;
  b->data = tmp;
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
                                    tile->z,
                                    tile);
}
