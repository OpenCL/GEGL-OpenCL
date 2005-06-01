/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#ifndef __GEGL_TILE_H__
#define __GEGL_TILE_H__

#include "gegl-object.h"

G_BEGIN_DECLS


#define GEGL_TYPE_TILE            (gegl_tile_get_type ())
#define GEGL_TILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE, GeglTile))
#define GEGL_TILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE, GeglTileClass))
#define GEGL_IS_TILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE))
#define GEGL_IS_TILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE))
#define GEGL_TILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE, GeglTileClass))


typedef struct _GeglTile      GeglTile;
typedef struct _GeglTileClass GeglTileClass;

struct _GeglTile
{
  GeglObject  parent_instance;

  /* x_offset and y_offset represent the origin of this tile
     in the image's coordinate system.
  */
  gint        x_offset;
  gint        y_offset;

  /* The sample_model, color_model and color_space are just references
   * to the containing images color_model, sample_model, and
   * color_space.  They are read only.
  */
  const GeglSampleModel *sample_model;
  const GeglColorModel  *color_model;
  const GeglColorSpace  *color_space;

  /* <private> */
  GeglBuffer            *buffer;

};

struct _GeglTileClass
{
  GeglObjectClass  parent_class;

  GeglTileIterator * (* get_iterator) (const GeglTile *self,
                                       const GeglRect *rect);
};


GType              gegl_tile_get_type            (void) G_GNUC_CONST;

GeglTileIterator * gegl_tile_get_iterator        (const GeglTile  *self,
                                                  const GeglRect  *rect);
GeglRect           gegl_tile_get_bounds          (const GeglTile  *self);
GeglTile         * gegl_tile_init_shared         (GeglTile        *source,
                                                  gint             x_offset,
                                                  gint             y_offset);
GeglTile         * gegl_tile_init                (gint             x_offset,
                                                  gint             y_offset,
                                                  GeglSampleModel *sample_model,
                                                  GeglColorModel  *color_model,
                                                  GeglColorSpace  *color_space,
                                                  GeglBuffer      *buffer);
GeglBuffer       * gegl_tile_get_writable_buffer (GeglTile        *self);
const GeglBuffer * gegl_tile_get_buffer          (const GeglTile  *self);


G_END_DECLS

#endif /* __GEGL_TILE_H__ */
