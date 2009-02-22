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

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation-point-render.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-node.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"

static gboolean gegl_operation_point_render_process 
                              (GeglOperation       *operation,
                               GeglBuffer          *output,
                               const GeglRectangle *result);

G_DEFINE_TYPE (GeglOperationPointRender, gegl_operation_point_render, GEGL_TYPE_OPERATION_SOURCE)

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
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
}

static void
gegl_operation_point_render_init (GeglOperationPointRender *self)
{
}


static gboolean
gegl_operation_point_render_process (GeglOperation       *operation,
                                     GeglBuffer          *output,
                                     const GeglRectangle *result)
{
  GeglPad    *pad;
  const Babl *out_format;
  GeglOperationPointRenderClass *point_render_class;

  point_render_class = GEGL_OPERATION_POINT_RENDER_GET_CLASS (operation);

  pad        = gegl_node_get_pad (operation->node, "output");
  out_format = gegl_pad_get_format (pad);
  if (!out_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (out_format);

  if ((result->width > 0) && (result->height > 0))
    {
      GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, out_format, GEGL_BUFFER_WRITE);      

      while (gegl_buffer_iterator_next (i))
          point_render_class->process (operation, i->data[0], i->length, &i->roi[0]);
    }
  return TRUE;
}
