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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_enum (sampler_type, _("Sampler"), GeglSamplerType, GEGL_TYPE_SAMPLER_TYPE,
                 GEGL_SAMPLER_CUBIC, _("Sampler used internaly"))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "map-absolute.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"


static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

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
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO           *o = GEGL_CHANT_PROPERTIES (operation);
  const Babl           *format_io, *format_coords;
  GeglSampler          *sampler;
  GeglBufferIterator   *it;
  gint                  index_in, index_out, index_coords;

  format_io = babl_format ("RGBA float");
  format_coords = babl_format_n (babl_type ("float"), 2);

  sampler = gegl_buffer_sampler_new (input, format_io, o->sampler_type);

  if (aux != NULL)
    {
      it = gegl_buffer_iterator_new (output, result, level, format_io, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
      index_out = 0;

      index_coords = gegl_buffer_iterator_add (it, aux, result, level, format_coords, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
      index_in = gegl_buffer_iterator_add (it, input, result, level, format_io, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

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
                  gegl_sampler_get (sampler, coords[0], coords[1], NULL, out);
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

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:map-absolute",
    "categories" , "transform",
    "description", _("sample input with an auxiliary buffer that contain absolute source coordinates"),
    NULL);
}
#endif
