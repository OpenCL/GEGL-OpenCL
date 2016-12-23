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
 * Copyright 2006 Øyvind Kolås
 */


#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-operation-point-render.h"
#include "gegl-operation-context.h"
#include "gegl-types-internal.h"

static gboolean gegl_operation_point_render_process
                              (GeglOperation       *operation,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

G_DEFINE_TYPE (GeglOperationPointRender, gegl_operation_point_render, GEGL_TYPE_OPERATION_SOURCE)

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_babl_rgba_linear_float ();
  gegl_operation_set_format (operation, "output", format);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  return NULL;
}

static void
gegl_operation_point_render_class_init (GeglOperationPointRenderClass *klass)
{
  GeglOperationSourceClass *source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);

  source_class->process    = gegl_operation_point_render_process;
  operation_class->prepare = prepare;

  operation_class->detect = detect;
  operation_class->no_cache = FALSE;
  operation_class->get_cached_region = NULL; /* we are able to compute anything
                                                 anywhere when we're our kind
                                                 of class */
  operation_class->threaded = TRUE;
}

static void
gegl_operation_point_render_init (GeglOperationPointRender *self)
{
}


static gboolean
gegl_operation_point_render_process (GeglOperation       *operation,
                                     GeglBuffer          *output,
                                     const GeglRectangle *result,
                                     gint                 level)
{
  const Babl *out_format;
  GeglOperationPointRenderClass *point_render_class;

  point_render_class = GEGL_OPERATION_POINT_RENDER_GET_CLASS (operation);

  out_format = gegl_operation_get_format (operation, "output");

  if (!out_format)
    {
      g_warning ("No output format set for %s", GEGL_OPERATION_GET_CLASS (operation)->name);
      return FALSE;
    }

  if ((result->width > 0) && (result->height > 0))
    {
      GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format,
                                                        GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (i))
          point_render_class->process (operation, i->data[0], i->length, &i->roi[0], level);
    }

  return TRUE;
}
