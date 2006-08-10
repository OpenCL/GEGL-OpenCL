/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
#include <string.h>

static gboolean evaluate_inner       (GeglOperation *operation,
                                      const gchar   *output_pad);

G_DEFINE_TYPE (GeglOperationPointFilter, gegl_operation_point_filter, GEGL_TYPE_OPERATION_FILTER)


static void
gegl_operation_point_filter_class_init (GeglOperationPointFilterClass * klass)
{
  /*GObjectClass       *object_class    = G_OBJECT_CLASS (klass);*/
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->evaluate = evaluate_inner;
}

static void
gegl_operation_point_filter_init (GeglOperationPointFilter *self)
{
  self->format = babl_format ("RGBA float"); /* default to RGBA float for
                                                processing */
}

static gboolean
evaluate_inner (GeglOperation *operation,
                const gchar   *output_pad)
{
  GeglOperationFilter *filter = GEGL_OPERATION_FILTER (operation);
  GeglOperationPointFilter *point_filter = GEGL_OPERATION_POINT_FILTER (operation);

  GeglBuffer *input  = filter->input;
  GeglRect   *result = gegl_operation_result_rect (operation);
  GeglBuffer *output;

  if(strcmp("output", output_pad))
    return FALSE;

  {
    gfloat *buf;

    g_assert (gegl_buffer_get_format (input));

    output = g_object_new (GEGL_TYPE_BUFFER,
                           "format", point_filter->format,
                           "x",      result->x,
                           "y",      result->y,
                           "width",  result->w,
                           "height", result->h,
                           NULL);

    if ( (result->w>0) && (result->h>0) )
      {
        buf  = g_malloc (4 * sizeof (gfloat) * gegl_buffer_pixels (output));

          {
            GeglBuffer *roi = g_object_new (GEGL_TYPE_BUFFER,
                                            "source", input,
                                            "x",      result->x,
                                            "y",      result->y,
                                            "width",  result->w,
                                            "height", result->h,
                                            NULL);
            gegl_buffer_get_fmt (roi, buf, point_filter->format);
            g_object_unref (roi);
          }
          {
            GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->evaluate (
               operation,
               buf,
               buf,
               gegl_buffer_pixels (output));
          }

          {
            GeglBuffer *roi = g_object_new (GEGL_TYPE_BUFFER,
                                            "source", output,
                                            "x",      result->x,
                                            "y",      result->y,
                                            "width",  result->w,
                                            "height", result->h,
                                            NULL);
            gegl_buffer_set_fmt (roi, buf, point_filter->format);
            g_object_unref (roi);
          }

        g_free (buf);
      }

    if (filter->output)
      g_object_unref (filter->output);
    filter->output = output;
  }
  return  TRUE;
}
