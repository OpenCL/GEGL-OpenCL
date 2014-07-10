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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_CONFIG_H__
#define __GEGL_CONFIG_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CONFIG, GeglConfigClass))
#define GEGL_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CONFIG))
#define GEGL_CONFIG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CONFIG, GeglConfigClass))
/* The rest is in gegl-types.h */

typedef struct _GeglConfigClass GeglConfigClass;

struct _GeglConfig
{
  GObject  parent_instance;

  gchar   *swap;
  guint64  tile_cache_size;
  gint     chunk_size; /* The size of elements being processed at once */
  gdouble  quality;
  gint     tile_width;
  gint     tile_height;
  gboolean use_opencl;
  gint     queue_size;
  gchar   *application_license;
};

struct _GeglConfigClass
{
  GObjectClass parent_class;
};

extern gint _gegl_threads;
#define gegl_config_threads()  (_gegl_threads)

#define GEGL_MAX_THREADS 16

G_END_DECLS

#endif
