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

#ifndef __GEGL_TILE_BACKEND_SWAPFILE_H__
#define __GEGL_TILE_BACKEND_SWAPFILE_H__

#include "gegl-tile-backend.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_SWAPFILE            (gegl_tile_backend_swapfile_get_type ())
#define GEGL_TILE_BACKEND_SWAPFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_SWAPFILE, GeglTileBackendSwapfile))
#define GEGL_TILE_BACKEND_SWAPFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_SWAPFILE, GeglTileBackendSwapfileClass))
#define GEGL_IS_TILE_BACKEND_SWAPFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_SWAPFILE))
#define GEGL_IS_TILE_BACKEND_SWAPFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_SWAPFILE))
#define GEGL_TILE_BACKEND_SWAPFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_SWAPFILE, GeglTileBackendSwapfileClass))


typedef struct _GeglTileBackendSwapfile      GeglTileBackendSwapfile;
typedef struct _GeglTileBackendSwapfileClass GeglTileBackendSwapfileClass;

struct _GeglTileBackendSwapfile
{
  GeglTileBackend  parent_instance;

  gchar           *path;
  gint             fd;
  GHashTable      *entries;
  GSList          *free_list;
  guint            next_unused;
  guint            total;
};

struct _GeglTileBackendSwapfileClass
{
  GeglTileBackendClass parent_class;
};

GType gegl_tile_backend_swapfile_get_type (void) G_GNUC_CONST;

void  gegl_tile_backend_swapfile_stats    (void);

G_END_DECLS

#endif
