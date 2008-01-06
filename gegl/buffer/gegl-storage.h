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

#ifndef __GEGL_STORAGE_H__
#define __GEGL_STORAGE_H__

#include "gegl-handlers.h"

#define GEGL_TYPE_STORAGE            (gegl_storage_get_type ())
#define GEGL_STORAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_STORAGE, GeglStorage))
#define GEGL_STORAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STORAGE, GeglStorageClass))
#define GEGL_IS_STORAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_STORAGE))
#define GEGL_IS_STORAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STORAGE))
#define GEGL_STORAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STORAGE, GeglStorageClass))

struct _GeglStorage
{
  GeglHandlers parent_instance;

  Babl        *format;
  gint         tile_width;
  gint         tile_height;
  gint         tile_size;
  gint         px_size;
  gint         width;
  gint         height;
  gchar       *path;

  guint        idle_swapper;
};

struct _GeglStorageClass
{
  GeglHandlersClass parent_class;
};

GType gegl_storage_get_type (void) G_GNUC_CONST;

#endif
