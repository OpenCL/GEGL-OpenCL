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
#ifndef _GEGL_TILE_DISK_H
#define _GEGL_TILE_DISK_H

#include <glib.h>
#include "gegl-buffer-types.h"
#include "gegl-tile-backend.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_DISK_STORE            (gegl_tile_disk_get_type ())
#define GEGL_TILE_DISK(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_DISK_STORE, GeglTileDisk))
#define GEGL_TILE_DISK_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_DISK_STORE, GeglTileDiskClass))
#define GEGL_IS_TILE_DISK_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_DISK_STORE))
#define GEGL_IS_TILE_DISK_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_DISK_STORE))
#define GEGL_TILE_DISK_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_DISK_STORE, GeglTileDiskClass))


typedef struct _GeglTileDisk      GeglTileDisk;
typedef struct _GeglTileDiskClass GeglTileDiskClass;

struct _GeglTileDisk
{
  GeglTileBackend  parent_instance;

  gchar      *path;
  gint        fd;
  GHashTable *entries;
  GSList     *free_list;
  guint       next_unused;
  guint       total;
};

struct _GeglTileDiskClass
{
  GeglTileBackendClass parent_class;
};

GType gegl_tile_disk_get_type (void) G_GNUC_CONST;

void gegl_tile_disk_stats (void);

G_END_DECLS

#endif
