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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */
#include "gegl-operation-point-filter.h"
#include "graph/gegl-pad.h"
#include <string.h>

static gboolean process_inner (GeglOperation *operation,
                               gpointer       context_id);

G_DEFINE_TYPE (GeglOperationPointFilter, gegl_operation_point_filter, GEGL_TYPE_OPERATION_FILTER)

static void prepare (GeglOperation *operation,
                     gpointer       context_id)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
gegl_operation_point_filter_class_init (GeglOperationPointFilterClass *klass)
{
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  filter_class->process = process_inner;
  operation_class->prepare = prepare;
}

static void
gegl_operation_point_filter_init (GeglOperationPointFilter *self)
{
}

static gboolean
process_inner (GeglOperation *operation,
               gpointer       context_id)
{
  GeglRectangle  *result = gegl_operation_result_rect (operation, context_id);
  GeglBuffer     *input;
/*  = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));*/
  GeglBuffer     *output;
  GeglPad        *pad;
  Babl           *in_format;
  Babl           *out_format;

  input = gegl_operation_get_source (operation, context_id, "input");

  pad       = gegl_node_get_pad (operation->node, "input");
  in_format = pad->format;
  if (!in_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (in_format);

  pad        = gegl_node_get_pad (operation->node, "output");
  out_format = pad->format;
  if (!out_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (out_format);

  output = gegl_operation_get_target (operation, context_id, "output");

  if ((result->width > 0) && (result->height > 0))
    {
    /* eek: this fails,..
      if (!(input->width == output->width &&
          input->height == output->height))
        {
          g_warning ("%ix%i -> %ix%i", input->width, input->height, output->width, output->height);
        }
      g_assert (input->width == output->width &&
                input->height == output->height);
     */
      if (in_format == out_format)
        {
          gfloat *buf;
          buf = g_malloc (in_format->format.bytes_per_pixel *
                          output->extent.width * output->extent.height);

          gegl_buffer_get (input, 1.0, result, in_format, buf, GEGL_AUTO_ROWSTRIDE);

          GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->process (
            operation,
            buf,
            buf,
            output->extent.width * output->extent.height);

          gegl_buffer_set (output, result, out_format, buf);
          g_free (buf);
        }
      else
        {
          gfloat *in_buf;
          gfloat *out_buf;
          in_buf = g_malloc (in_format->format.bytes_per_pixel *
                             input->extent.width * input->extent.height);
          out_buf = g_malloc (out_format->format.bytes_per_pixel *
                             output->extent.width * output->extent.height);

          gegl_buffer_get (input, 1.0, result, in_format, in_buf, GEGL_AUTO_ROWSTRIDE);

          GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->process (
            operation,
            in_buf,
            out_buf,
            output->extent.width * output->extent.height);

          gegl_buffer_set (output, result, out_format, out_buf);
          g_free (in_buf);
          g_free (out_buf);
        }
    }
  gegl_buffer_destroy (input);
  return TRUE;
}
