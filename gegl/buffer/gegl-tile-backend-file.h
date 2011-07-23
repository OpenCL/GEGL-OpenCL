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

#ifndef __GEGL_TILE_BACKEND_FILE_H__
#define __GEGL_TILE_BACKEND_FILE_H__

#include "gegl-tile-backend.h"

/***
 * GeglTileBackendFile is a GeglTileBackend that store tiles in a unique file.
 */

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_FILE            (gegl_tile_backend_file_get_type ())
#define GEGL_TILE_BACKEND_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFile))
#define GEGL_TILE_BACKEND_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFileClass))
#define GEGL_IS_TILE_BACKEND_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_FILE))
#define GEGL_IS_TILE_BACKEND_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_FILE))
#define GEGL_TILE_BACKEND_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFileClass))


typedef struct _GeglTileBackendFile      GeglTileBackendFile;
typedef struct _GeglTileBackendFileClass GeglTileBackendFileClass;


struct _GeglTileBackendFileClass
{
  GeglTileBackendClass parent_class;
};

GType gegl_tile_backend_file_get_type (void) G_GNUC_CONST;

void  gegl_tile_backend_file_stats    (void);

gboolean gegl_tile_backend_file_try_lock (GeglTileBackendFile *file);
gboolean gegl_tile_backend_file_unlock   (GeglTileBackendFile *file);

G_END_DECLS

#endif
