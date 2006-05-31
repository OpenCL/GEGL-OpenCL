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
#ifndef _GEGL_TILE_ABYSS_H
#define _GEGL_TILE_ABYSS_H

#include <glib.h>
#include "gegl-tile.h"
#include "gegl-tile-trait.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_ABYSS            (gegl_tile_abyss_get_type ())
#define GEGL_TILE_ABYSS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_ABYSS, GeglTileAbyss))
#define GEGL_TILE_ABYSS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_ABYSS, GeglTileAbyssClass))
#define GEGL_IS_TILE_ABYSS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_ABYSS))
#define GEGL_IS_TILE_ABYSS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_ABYSS))
#define GEGL_TILE_ABYSS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_ABYSS, GeglTileAbyssClass))


typedef struct _GeglTileAbyss      GeglTileAbyss;
typedef struct _GeglTileAbyssClass GeglTileAbyssClass;

struct _GeglTileAbyss
{
  GeglTileTrait    parent_instance;

  /* private */
  GeglTile        *tile;
  GeglTileBackend *backend;
  gint             x;  /*< clipping rect */
  gint             y;
  gint             width;
  gint             height;
};

struct _GeglTileAbyssClass
{
  GeglTileTraitClass parent_class;
};

GType gegl_tile_abyss_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
