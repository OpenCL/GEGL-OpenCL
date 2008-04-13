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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_GIO_H__
#define __GEGL_TILE_GIO_H__

#include "gegl-tile-backend.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_GIO            (gegl_tile_gio_get_type ())
#define GEGL_TILE_GIO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_GIO, GeglTileGio))
#define GEGL_TILE_GIO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_GIO, GeglTileGioClass))
#define GEGL_IS_TILE_GIO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_GIO))
#define GEGL_IS_TILE_GIO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_GIO))
#define GEGL_TILE_GIO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_GIO, GeglTileGioClass))

typedef struct _GeglTileGio      GeglTileGio;
typedef struct _GeglTileGioClass GeglTileGioClass;

struct _GeglTileGio
{
  GeglTileBackend  parent_instance;

  gchar           *path;         /* the base path of the buffer */
  GFile           *buffer_dir;
};

struct _GeglTileGioClass
{
  GeglTileBackendClass parent_class;
};

GType gegl_tile_gio_get_type (void) G_GNUC_CONST;

void  gegl_tile_gio_stats    (void);

G_END_DECLS

#endif
