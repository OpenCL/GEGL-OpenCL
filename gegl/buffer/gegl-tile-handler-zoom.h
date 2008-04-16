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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_HANDLER_ZOOM_H__
#define __GEGL_TILE_HANDLER_ZOOM_H__

#include "gegl-tile-handler.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_HANDLER_ZOOM            (gegl_tile_handler_zoom_get_type ())
#define GEGL_TILE_HANDLER_ZOOM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_HANDLER_ZOOM, GeglTileHandlerZoom))
#define GEGL_TILE_HANDLER_ZOOM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_HANDLER_ZOOM, GeglTileHandlerZoomClass))
#define GEGL_IS_TILE_HANDLER_ZOOM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_HANDLER_ZOOM))
#define GEGL_IS_TILE_HANDLER_ZOOM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_HANDLER_ZOOM))
#define GEGL_TILE_HANDLER_ZOOM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_HANDLER_ZOOM, GeglTileHandlerZoomClass))


typedef struct _GeglTileHandlerZoom      GeglTileHandlerZoom;
typedef struct _GeglTileHandlerZoomClass GeglTileHandlerZoomClass;

struct _GeglTileHandlerZoom
{
  GeglTileHandler  parent_instance;

  GeglTileBackend *backend;
  GeglTileStorage *tile_storage;
};

struct _GeglTileHandlerZoomClass
{
  GeglTileHandlerClass  parent_class;
};

GType gegl_tile_handler_zoom_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
