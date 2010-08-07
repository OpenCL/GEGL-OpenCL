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


#ifdef GEGL_CHANT_PROPERTIES

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "render_mapping.c"

#include "gegl-chant.h"


static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", babl_format_n (babl_type ("float"), 2));
  gegl_operation_set_format (operation, "output", format);
}

static void
get_required_for_output (GeglOperation *operation)
{
  //TODO
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
  gint                  index_out, index_coords;
  
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
    
    while (gegl_buffer_iterator_next (it))
    {
      gint        i;
      gint        n_pixels = it->length;
      gfloat     *out = it->data[index_out];
      gfloat     *coords = it->data[index_coords];
      gfloat     *sample;
      
      for (i=0; i<n_pixels; i++)
      {
        /* FIXME coords[0] > 0 for test, need to be coords[0] >= 0 */
        if (coords[0] > 0 && coords[1] > 0)
        {
          sample = gegl_sampler_get_from_buffer (sampler, coords[0], coords[1]);
          
          out[0] = sample[0];
          out[1] = sample[1];
          out[2] = sample[2];
          out[3] = sample[3];
        }
        else
        {
          out[0] = 0.0;
          out[1] = 0.0;
          out[2] = 0.0;
          out[3] = 0.0;
        }
        
        coords += 2;
        out += 4;
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
  
  operation_class->name        = "gegl:render_mapping";
  
  operation_class->categories  = "transform";
  operation_class->description = _("sample input with an auxiliar buffer that contain source coordinates");
}
#endif
