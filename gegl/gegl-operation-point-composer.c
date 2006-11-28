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
#include "gegl-operation-point-composer.h"
#include <string.h>

static gboolean process_inner (GeglOperation *operation);

G_DEFINE_TYPE (GeglOperationPointComposer, gegl_operation_point_composer, GEGL_TYPE_OPERATION_COMPOSER)


static void
gegl_operation_point_composer_class_init (GeglOperationPointComposerClass * klass)
{
  /*GObjectClass       *object_class    = G_OBJECT_CLASS (klass);*/
  GeglOperationComposerClass *composer_class = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process_inner;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{
  self->format = babl_format ("RGBA float"); /* default to RGBA float for
                                                processing */
  self->aux_format = babl_format ("RGBA float"); /* default to RGBA float for
                                                    processing */
}

static gboolean
process_inner (GeglOperation *operation)
{
  GeglOperationComposer *composer = GEGL_OPERATION_COMPOSER (operation);
  GeglOperationPointComposer *point_composer = GEGL_OPERATION_POINT_COMPOSER (operation);

  GeglBuffer *input  = composer->input;
  GeglBuffer *aux    = composer->aux;
  GeglRect   *result = gegl_operation_result_rect (operation);
  GeglBuffer *output;

  if (!input && aux)
    {
        composer->output = g_object_ref (aux);
        return TRUE;
    }

  {
    gfloat *buf = NULL, *aux_buf = NULL;

    g_assert (gegl_buffer_get_format (input));

    output = g_object_new (GEGL_TYPE_BUFFER,
                           "format", point_composer->format,
                           "x",      result->x,
                           "y",      result->y,
                           "width",  result->w,
                           "height", result->h,
                           NULL);

    if ( (result->w>0) && (result->h>0))
      {
        buf = g_malloc (4 * sizeof (gfloat) * gegl_buffer_pixels (output));

        if (aux)
          aux_buf = g_malloc (4 * sizeof (gfloat) * gegl_buffer_pixels (output));

        gegl_buffer_get (input, result, buf, point_composer->format, 1.0);
          
        if (aux)
          {
            gegl_buffer_get (aux, result, aux_buf, point_composer->aux_format, 1.0);
          }
          {
            GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
               operation,
               buf,
               aux_buf,
               buf,
               gegl_buffer_pixels (output));
          }

        gegl_buffer_set (output, result, buf, point_composer->format);

        g_free (buf);
        if (aux)
          g_free (aux_buf);

        }
        composer->output = output;
      }
  return  TRUE;
}
