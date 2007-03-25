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
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "gegl-tile-traits.h"
#include "gegl-tile-cache.h"

G_DEFINE_TYPE (GeglTileTraits, gegl_tile_traits, GEGL_TYPE_TILE_TRAIT)
static GObjectClass * parent_class = NULL;

static void
gegl_tile_traits_rebind (GeglTileTraits *traits);

static void
gegl_tile_traits_nuke_cache (GeglTileTraits *traits)
{
  GSList *iter;

  while (gegl_tile_traits_get_first (traits, GEGL_TYPE_TILE_CACHE))
    {
      iter = traits->chain;
      while (iter)
        {
          if (GEGL_IS_TILE_CACHE (iter->data))
            {
              g_object_unref (iter->data);
              traits->chain = g_slist_remove (traits->chain, iter->data);
              gegl_tile_traits_rebind (traits);
              break;
            }
          iter = iter->next;
        }
    }
}

static void
dispose (GObject *object)
{
  GeglTileTraits *traits = GEGL_TILE_TRAITS (object);
  GSList         *iter;

  /* Get rid of the cache before any further parts of the deconstruction of the
   * TileStore chain, unwritten tiles need a living TileStore for their
   * deconstruction.
   */
  gegl_tile_traits_nuke_cache (traits);

  iter = traits->chain;
  while (iter)
    {
      if (iter->data)
        g_object_unref (iter->data);
      iter = iter->next;
    }

  if (traits->chain)
    g_slist_free (traits->chain);
  traits->chain = NULL;

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
}


static void
finalize (GObject *object)
{
  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static GeglTile *
get_tile (GeglTileStore *tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglTileTraits *traits = GEGL_TILE_TRAITS (tile_store);
  GeglTileStore  *source = GEGL_TILE_TRAIT (tile_store)->source;
  GeglTile       *tile   = NULL;

  if (traits->chain != NULL)
    tile = gegl_tile_store_get_tile (GEGL_TILE_STORE (traits->chain->data),
                                     x, y, z);
  else if (source)
    tile = gegl_tile_store_get_tile (source, x, y, z);
  else
    g_assert (0);
  return tile;
}

static gboolean
message (GeglTileStore  *tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglTileTraits *traits = GEGL_TILE_TRAITS (tile_store);
  GeglTileStore  *source = GEGL_TILE_TRAIT (tile_store)->source;

  if (traits->chain != NULL)
    return gegl_tile_store_message (GEGL_TILE_STORE (traits->chain->data), message, x, y, z, data);
  else if (source)
    return gegl_tile_store_message (source, message, x, y, z, data);
  else
    g_assert (0);

  return FALSE;
}

static void
gegl_tile_traits_class_init (GeglTileTraitsClass *class)
{
  GObjectClass       *gobject_class;
  GeglTileStoreClass *tile_store_class;

  gobject_class    = (GObjectClass *) class;
  tile_store_class = (GeglTileStoreClass *) class;

  tile_store_class->get_tile = get_tile;
  tile_store_class->message  = message;

  parent_class            = g_type_class_peek_parent (class);
  gobject_class->finalize = finalize;
  gobject_class->dispose  = dispose;
}

static void
gegl_tile_traits_init (GeglTileTraits *self)
{
  self->chain = NULL;
}

GeglTileStore *tsource = NULL;

static void
gegl_tile_traits_rebind (GeglTileTraits *traits)
{
  GSList *iter;


  iter = traits->chain;
  while (iter)
    {
      GeglTileTrait *trait;
      GeglTileStore *source = NULL;

      trait = iter->data;
      if (iter->next)
        {
          source = g_object_ref (iter->next->data);
        }
      else
        {
          g_object_get (traits, "source", &source, NULL);
        }
      g_object_set (G_OBJECT (trait), "source", source, NULL);
      g_object_unref (source);
      iter = iter->next;
    }
}

GeglTileTrait *
gegl_tile_traits_add (GeglTileTraits *traits,
                      GeglTileTrait  *trait)
{
  tsource       = GEGL_TILE_STORE (GEGL_TILE_TRAIT (traits)->source);
  traits->chain = g_slist_prepend (traits->chain, trait);
  gegl_tile_traits_rebind (traits);
  tsource = NULL;
  return trait;
}

/*
 * return the first trait of a given type
 */
GeglTileTrait *
gegl_tile_traits_get_first (GeglTileTraits *traits,
                            GType           type)
{
  GSList *iter;

  iter = traits->chain;
  while (iter)
    {
      if ((G_TYPE_CHECK_INSTANCE_TYPE ((iter->data), type)))
        {
          return iter->data;
        }
      iter = iter->next;
    }
  return NULL;
}

