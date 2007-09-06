/* This file is an image processing operation for GEGL
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
 * Copyright 2007 Øyvind Kolås <oeyvindk@hig.no>
 */

#if GEGL_CHANT_PROPERTIES 

#else

#define GEGL_CHANT_NAME        hstack
#define GEGL_CHANT_SELF        "hstack.c"
#define GEGL_CHANT_DESCRIPTION "Horizontally stack inputs, (in \"output\" \"aux\" is placed to the right of \"input\")"
#define GEGL_CHANT_CATEGORIES  "misc"

#define GEGL_CHANT_COMPOSER
#define GEGL_CHANT_CLASS_INIT

#include "gegl-chant.h"
#include <math.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationComposer *composer;
  GeglBuffer            *input;
  GeglBuffer            *aux;
  GeglBuffer            *output;
  GeglBuffer            *temp_in;
  GeglBuffer            *temp_aux;
  GeglRectangle         *result;
 

  composer = GEGL_OPERATION_COMPOSER (operation);
  result = gegl_operation_result_rect (operation, context_id);
  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  aux = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "aux"));

  /* FIXME: just pass the originals buffers if the result rectangle does not
   * include both input buffers
   */

          temp_in = gegl_buffer_create_sub_buffer (input, result);
          temp_aux = gegl_buffer_create_sub_buffer (aux, result);
          output = gegl_buffer_new (result, babl_format ("RGBA float"));

    {
      gfloat *buf = g_malloc0 (result->width * result->height * 4 * 4);
      gfloat *bufB = g_malloc0 (result->width * result->height * 4 * 4);

      gegl_buffer_get (temp_in, NULL, 1.0, babl_format ("RGBA float"), buf);
      gegl_buffer_get (temp_aux, NULL, 1.0, babl_format ("RGBA float"), bufB);
        {
          gint offset=0;
          gint x,y;
          for (y=0;y<output->height;y++)
            for (x=0;x<output->width;x++)
              {
                if (x + result->x >= input->width)
                  {
                    buf[offset+0]=bufB[offset+0];
                    buf[offset+1]=bufB[offset+1];
                    buf[offset+2]=bufB[offset+2];
                    buf[offset+3]=bufB[offset+3];
                  }
                offset+=4;
              }
        }
      gegl_buffer_set (output, NULL, babl_format ("RGBA float"), buf);

      g_free (buf);
      g_free (bufB);
    }
  g_object_unref (temp_in);
  g_object_unref (temp_aux);
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                     "input");
  GeglRectangle *aux_rect = gegl_operation_source_get_defined_region (operation,
                                                                      "aux");

  if (!in_rect)
    return result;

  result = *in_rect;
  if (result.width  != 0 &&
      result.height  != 0)
    {
      result.width += aux_rect->width;
      if (aux_rect->height > result.height)
        result.height = aux_rect->height;
    }
  
  return result;
}

static GeglRectangle
compute_input_request (GeglOperation *self,
                       const gchar   *input_pad,
                       GeglRectangle *roi)
{
  GeglRectangle request = *roi;

  if (!strcmp (input_pad, "aux"))
    {
      GeglRectangle *in_rect = gegl_operation_source_get_defined_region (self,
                                                                         "input");
      GeglRectangle *aux_rect = gegl_operation_source_get_defined_region (self,
                                                                         "aux");

      if (request.width != 0 &&
          request.height != 0)
        {
          request.x -= in_rect->width + aux_rect->x;
        }
    }

  return request;
}

static GeglRectangle
compute_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  if (!strcmp ("input_pad", "input"))
    {
      return region;
    }
  else
    {
      /*region.x -= radius * 2;*/
    } 
  return region;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region      = get_defined_region;
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->compute_input_request   = compute_input_request;
}

#endif
