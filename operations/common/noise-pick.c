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
 * Copyright 1997 Miles O'Neal <meo@rru.com>  http://www.rru.com/~meo/
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 * Copyright 2013 Téo Mazars   <teo.mazars@ensimag.fr>
 */

/*
 *  PICK Operation
 *     We pick a pixel at random from the neighborhood of the current pixel.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (pct_random, _("Randomization (%)"), 50.0)
    value_range (0.0, 100.0)

property_int    (repeat, _("Repeat"), 1)
    value_range (1, 100)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     noise_pick
#define GEGL_OP_C_SOURCE noise-pick.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  const Babl              *format;

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->repeat;

  format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}


/* We avoid unecessary calls to gegl_random
 * by recomputing a small part ourselves.
 * see gegl/gegl-random.c for details */
#define RAND_UINT_TO_FLOAT(x) (((x) & 0xffff) * 0.00001525902189669642175)

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o;
  const Babl         *format;
  gint                bpp;
  GeglBufferIterator *gi;

  o = GEGL_PROPERTIES (operation);

  format = gegl_operation_get_source_format (operation, "input");
  bpp = babl_format_get_bytes_per_pixel (format);

  gi = gegl_buffer_iterator_new (output, result, 0, format,
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (gi))
    {
      gchar        *data = gi->data[0];
      GeglRectangle roi  = gi->roi[0];
      gint          i, j;

      for (j = roi.y; j < roi.y + roi.height ; j++)
        for (i = roi.x; i < roi.x + roi.width ; i++)
          {
            gint r;
            gint pos_x = i, pos_y = j;

            for (r = 0; r < o->repeat; r++)
              {
                guint  rand = gegl_random_int (o->rand, pos_x, pos_y, 0, r);
                gfloat pct  = RAND_UINT_TO_FLOAT (rand) * 100.0;

                if (pct <= o->pct_random)
                  {
                    gint rand2 = (gint) (rand % 9);

                    pos_x += (rand2 % 3) - 1;
                    pos_y += (rand2 / 3) - 1;
                  }
              }

            gegl_buffer_sample_at_level (input, pos_x, pos_y, NULL, data, format,
                                level,
                                GEGL_SAMPLER_NEAREST, GEGL_ABYSS_CLAMP);
            data += bpp;
          }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-pick",
    "title",      _("Noise Pick"),
    "reference-hash", "13aa5200ceff0b5607fb7ae867bf9d79",
    "categories", "noise",
    "description", _("Randomly interchange some pixels with neighbors"),
    NULL);
}

#endif
