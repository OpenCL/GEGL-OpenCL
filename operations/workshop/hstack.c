/* This file is an image processing operation for GEGL
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
 * Copyright 2007 Øyvind Kolås <oeyvindk@hig.no>
 */

#ifdef GEGL_CHANT_PROPERTIES

    /* No properties */

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "hstack.c"

#include "gegl-chant.h"
#include <math.h>

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
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
compute_input_request (GeglOperation       *self,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
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
compute_affected_region (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  if (!strcmp ("input_pad", "input"))
    {
      return *region;
    }
  else
    {
      /*region.x -= radius * 2;*/
    }
  return *region;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglOperationComposer *composer;
  GeglBuffer            *temp_in;
  GeglBuffer            *temp_aux;

  composer = GEGL_OPERATION_COMPOSER (operation);

  /* FIXME: just pass the originals buffers if the result rectangle does not
   * include both input buffers
   */

  temp_in = gegl_buffer_create_sub_buffer (input, result);
  temp_aux = gegl_buffer_create_sub_buffer (aux, result);

    {
      gfloat *buf  = g_new0 (gfloat, result->width * result->height * 4);
      gfloat *bufB = g_new0 (gfloat, result->width * result->height * 4);

      gegl_buffer_get (temp_in, 1.0, NULL, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get (temp_aux, 1.0, NULL, babl_format ("RGBA float"), bufB, GEGL_AUTO_ROWSTRIDE);
        {
          gint offset=0;
          gint x,y;
          for (y=0;y<gegl_buffer_get_height (output);y++)
            for (x=0;x<gegl_buffer_get_width (output);x++)
              {
                if (x + result->x >= gegl_buffer_get_width (input))
                  {
                    buf[offset+0]=bufB[offset+0];
                    buf[offset+1]=bufB[offset+1];
                    buf[offset+2]=bufB[offset+2];
                    buf[offset+3]=bufB[offset+3];
                  }
                offset+=4;
              }
        }
      gegl_buffer_set (output, NULL, babl_format ("RGBA float"), buf,
                       GEGL_AUTO_ROWSTRIDE);

      g_free (buf);
      g_free (bufB);
    }
  g_object_unref (temp_in);
  g_object_unref (temp_aux);

  return  TRUE;
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_defined_region = get_defined_region;
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->compute_input_request   = compute_input_request;

  operation_class->name        = "hstack";
  operation_class->categories  = "misc";
  operation_class->description =
        "Horizontally stack inputs, (in \"output\" \"aux\" is placed to the right of \"input\")";
}

#endif
