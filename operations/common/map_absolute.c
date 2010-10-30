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
 * Copyright 2010 Michael Muré <batolettre@gmail.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <gegl-utils.h>
#include "buffer/gegl-sampler.h"
#include "buffer/gegl-buffer-iterator.h"

#include <stdio.h>

#ifdef GEGL_CHANT_PROPERTIES

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "map_absolute.c"

#include "gegl-chant.h"


static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", babl_format_n (babl_type ("float"), 2));
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                        const gchar         *input_pad,
                        const GeglRectangle *region)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  Babl                 *format_io, *format_coords;
  GeglSampler          *sampler;
  GType                 desired_type;
  GeglInterpolation     interpolation;
  GeglBufferIterator   *it;
  gint                  index_in, index_out, index_coords;
  
  format_io = babl_format ("RGBA float");
  format_coords = babl_format_n (babl_type ("float"), 2);
  
  interpolation = gegl_buffer_interpolation_from_string ("cubic");
  desired_type = gegl_sampler_type_from_interpolation (interpolation);
  
  sampler = g_object_new (desired_type,
                          "format", format_io,
                          "buffer", input,
                          NULL);
  
  gegl_sampler_prepare (sampler);  
  
  if (aux != NULL)
  {
    it = gegl_buffer_iterator_new (output, result, format_io, GEGL_BUFFER_WRITE);
    index_out = 0;
    
    index_coords = gegl_buffer_iterator_add (it, aux, result, format_coords, GEGL_BUFFER_READ);
    index_in = gegl_buffer_iterator_add (it, input, result, format_io, GEGL_BUFFER_READ);
    
    while (gegl_buffer_iterator_next (it))
    {
      gint        i;
      gint        n_pixels = it->length;
      gint        x = it->roi->x; /* initial x                   */
      gint        y = it->roi->y; /*           and y coordinates */
      gfloat     *in = it->data[index_in];
      gfloat     *out = it->data[index_out];
      gfloat     *coords = it->data[index_coords];
      
      for (i=0; i<n_pixels; i++)
      {
        if (coords[0] > 0 && coords[1] > 0)
        {
          /* if the coordinate asked is an exact pixel, we fetch it directly, to avoid the blur of sampling */
          if (coords[0] == x && coords[1] == y)
          {
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out[3] = in[3];
          }
          else
          {
            gegl_sampler_get (sampler, coords[0], coords[1], out);
          }
        }
        else
        {
          out[0] = 0.0;
          out[1] = 0.0;
          out[2] = 0.0;
          out[3] = 0.0;
        }
        
        coords += 2;
        in += 4;
        out += 4;
        
        /* update x and y coordinates */
        x++;
        if (x >= (it->roi->x + it->roi->width))
        {
          x = it->roi->x;
          y++;
        }

      }
    }
    
  }
  else
  {
    gegl_buffer_copy (input, result, output, result);
  }
  
  g_object_unref (sampler);
  
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  
  operation_class->name        = "gegl:map-absolute";
  
  operation_class->categories  = "transform";
  operation_class->description = _("sample input with an auxiliary buffer that contain source coordinates");
}
#endif
