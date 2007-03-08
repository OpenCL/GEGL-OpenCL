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

#define GEGL_CHANT_COMPOSER
#define GEGL_CHANT_NAME        hstack
#define GEGL_CHANT_DESCRIPTION "Horizontally stack inputs, (in \"output\" \"aux\" is placed to the right of \"input\")"
#define GEGL_CHANT_SELF        "hstack.c"
#define GEGL_CHANT_CATEGORIES  "misc"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"
#include <math.h>

#include <stdio.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationComposer *composer;
  GeglBuffer            *input;
  GeglBuffer            *aux;
  GeglBuffer            *output;

  composer = GEGL_OPERATION_COMPOSER (operation);

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  aux = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "aux"));
    {
      GeglRectangle   *result = gegl_operation_result_rect (operation, context_id);

      if (result->width == 0 ||
          result->height== 0)
        {
          output = g_object_ref (input);
        }
      else
        {
          /* FIXME: just pass the originals buffers if the result rectangle does not
           * include both input buffers
           */
          GeglBuffer      *temp_in;
          GeglBuffer      *temp_aux;

          temp_in = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", input,
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->width,
                                 "height", result->height,
                                 NULL);
          temp_aux = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", aux,
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->width,
                                 "height", result->height,
                                 "shift-x", -(input->width + input->x),
                                 "shift-y", -(input->y),
                                 NULL);
          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RGBA float"),
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->width,
                                 "height", result->height,
                                 NULL);
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
        }
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
    }
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

static GeglRectangle get_source_rect_in (GeglOperation *self,
                                         gpointer       context_id)
{
  GeglRectangle rect;
  rect  = *gegl_operation_get_requested_region (self, context_id);
  return rect;
}

static GeglRectangle get_source_rect_aux (GeglOperation *self,
                                          gpointer       context_id)
{
  GeglRectangle  rect;
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (self,
                                                                     "input");
  GeglRectangle *aux_rect = gegl_operation_source_get_defined_region (self,
                                                                     "aux");

  rect  = *gegl_operation_get_requested_region (self, context_id);
  if (rect.width  != 0 &&
      rect.height  != 0)
    {
      rect.x -= in_rect->width + aux_rect->x;
    }
  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       context_id)
{
  GeglRectangle need_in = get_source_rect_in (self, context_id);
  GeglRectangle need_aux = get_source_rect_aux (self, context_id);

  gegl_operation_set_source_region (self, context_id, "input", &need_in);
  gegl_operation_set_source_region (self, context_id, "aux", &need_aux);

  return TRUE;
}

static GeglRectangle
get_affected_region (GeglOperation *self,
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
  operation_class->get_defined_region  = get_defined_region;
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
