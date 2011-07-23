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

#ifndef __GEGL_TILE_BACKEND_RAM_H__
#define __GEGL_TILE_BACKEND_RAM_H__

#include "gegl-tile-backend.h"

/***
 * GeglTileBackendRam is a GeglTileBackend that store tiles in RAM.
 */

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_RAM            (gegl_tile_backend_ram_get_type ())
#define GEGL_TILE_BACKEND_RAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_RAM, GeglTileBackendRam))
#define GEGL_TILE_BACKEND_RAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_RAM, GeglTileBackendRamClass))
#define GEGL_IS_TILE_BACKEND_RAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_RAM))
#define GEGL_IS_TILE_BACKEND_RAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_RAM))
#define GEGL_TILE_BACKEND_RAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_RAM, GeglTileBackendRamClass))

typedef struct _GeglTileBackendRam      GeglTileBackendRam;
typedef struct _GeglTileBackendRamClass GeglTileBackendRamClass;

struct _GeglTileBackendRam
{
  GeglTileBackend  parent_instance;

  GHashTable      *entries;
};

struct _GeglTileBackendRamClass
{
  GeglTileBackendClass parent_class;
};

GType gegl_tile_backend_ram_get_type (void) G_GNUC_CONST;

void  gegl_tile_backend_ram_stats    (void);

G_END_DECLS

#endif
