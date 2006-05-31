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
#ifndef _TILE_BACKEND_H
#define _TILE_BACKEND_H

#include <glib.h>
#include "gegl-buffer-types.h"
#include "gegl-tile.h"
#include "gegl-tile-store.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND            (gegl_tile_backend_get_type ())
#define GEGL_TILE_BACKEND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND, GeglTileBackend))
#define GEGL_TILE_BACKEND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND, GeglTileBackendClass))
#define GEGL_IS_TILE_BACKEND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND))
#define GEGL_IS_TILE_BACKEND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND))
#define GEGL_TILE_BACKEND_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND, GeglTileBackendClass))

struct _GeglTileBackend
{
  GeglTileStore  parent_instance;
  gint           tile_width;
  gint           tile_height;
  void          *babl_format;   /* defaults to the babl format "rgba8" */
  gint           px_size;       /* size of a single pixel in bytes */
  gint           tile_size;     /* size of an entire tile in bytes */
};

struct _GeglTileBackendClass
{
  GeglTileStoreClass parent_class;
};

GType gegl_tile_backend_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
