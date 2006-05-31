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
#include <glib-object.h>

#include "gegl-buffer-types.h"
#include "gegl-tile.h"

#ifndef _GEGL_TILE_TRAITS_H
#define _GEGL_TILE_TRAITS_H

#define GEGL_TYPE_TILE_TRAITS           (gegl_tile_traits_get_type ())
#define GEGL_TILE_TRAITS(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_TRAITS, GeglTileTraits))
#define GEGL_TILE_TRAITS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_TRAITS, GeglTileTraitsClass))
#define GEGL_IS_TILE_TRAITS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_TRAITS))
#define GEGL_IS_TILE_TRAITS_CLASS(klass)(G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_TRAITS))
#define GEGL_TILE_TRAITS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_TRAITS, GeglTileTraitsClass))

#include "gegl-tile-trait.h"

struct _GeglTileTraits
{
  GeglTileTrait  parent_object;
  GSList        *chain;
};

struct _GeglTileTraitsClass
{
  GeglTileTraitClass parent_class;
};

GType   gegl_tile_traits_get_type           (void) G_GNUC_CONST;

/* NOTE: gegl_tile_traits_add steals the initial assumed reference */
GeglTileTrait * gegl_tile_traits_add        (GeglTileTraits *traits,
                                             GeglTileTrait  *trait);

/* returns the first matching trait of a specified type (or NULL) */
GeglTileTrait * gegl_tile_traits_get_first  (GeglTileTraits *traits,
                                             GType           type);

#endif
