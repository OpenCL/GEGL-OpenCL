/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Daniel Sabo
 */

 /*
  * This file contains alternate versions of functions to make them
  * more introspection friendly. They are not a public part of the
  * C API and should not be used outside of this file.
  */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "graph/gegl-node.h"
#include "gegl-introspection-support.h"

GeglRectangle *
gegl_node_introspectable_get_bounding_box (GeglNode *node)
{
  GeglRectangle  bbox   = gegl_node_get_bounding_box (node);
  GeglRectangle *result = g_new (GeglRectangle, 1);

  *result = bbox;

  return result;
}

GValue *
gegl_node_introspectable_get_property (GeglNode    *node,
                                       const gchar *property_name)
{
  GValue *result = g_new0 (GValue, 1);

  gegl_node_get_property (node, property_name, result);

  return result;
}

GeglBuffer *
gegl_buffer_introspectable_new (const char *format_name,
                                gint        x,
                                gint        y,
                                gint        width,
                                gint        height)
{
  const Babl *format = NULL;

  if (format_name)
    format = babl_format (format_name);

  if (!format)
    format = babl_format ("RGBA float");

  return g_object_new (GEGL_TYPE_BUFFER,
                       "x", x,
                       "y", y,
                       "width", width,
                       "height", height,
                       "format", format,
                       NULL);
}

guchar *
gegl_buffer_introspectable_get (GeglBuffer          *buffer,
                                const GeglRectangle *rect,
                                gdouble              scale,
                                const gchar         *format_name,
                                GeglAbyssPolicy      repeat_mode,
                                guint               *data_length)
{
  const Babl *format;
  guint bpp;
  guchar *result;

  *data_length = 0;

  if (format_name)
    format = babl_format (format_name);
  else
    format = gegl_buffer_get_format (buffer);

  if (rect->width <= 0 || rect->height <= 0)
    return NULL;
  if (scale <= 0.0)
    return NULL;

  bpp = babl_format_get_bytes_per_pixel (format);
  *data_length = bpp * rect->width * rect->height;

  result = g_malloc (*data_length);

  gegl_buffer_get (buffer, rect, scale, format, result, GEGL_AUTO_ROWSTRIDE, repeat_mode);

  return result;
}

void
gegl_buffer_introspectable_set (GeglBuffer          *buffer,
                                const GeglRectangle *rect,
                                const gchar         *format_name,
                                const guchar        *src,
                                gint                 src_length)
{
  const Babl *format;
  guint bpp;

  format = babl_format (format_name);

  if (rect->width <= 0 || rect->height <= 0)
    return;

  bpp = babl_format_get_bytes_per_pixel (format);

  g_return_if_fail (src_length == bpp * rect->width * rect->height);

  gegl_buffer_set (buffer, rect, 0, format, src, 0);
}
