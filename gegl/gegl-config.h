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

#define GEGL_TYPE_CONFIG            (gegl_config_get_type ())
#define GEGL_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CONFIG, GeglConfig))
#define GEGL_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CONFIG, GeglConfigClass))
#define GEGL_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CONFIG))
#define GEGL_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CONFIG))
#define GEGL_CONFIG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CONFIG, GeglConfigClass))

typedef struct _GeglConfigClass GeglConfigClass;

struct _GeglConfig
{
  GObject  parent_instance;

  gchar   *swap;
  gint     cache_size;
  gint     chunk_size; /* The size of elements being processed at once */
  gdouble  quality;
  gdouble  babl_tolerance;
  gint     tile_width;
  gint     tile_height;
  gint     threads;
  gboolean use_opencl;
};

struct _GeglConfigClass
{
  GObjectClass parent_class;
};

GType gegl_config_get_type (void) G_GNUC_CONST;

GeglConfig   * gegl_config            (void);

G_END_DECLS

#endif
