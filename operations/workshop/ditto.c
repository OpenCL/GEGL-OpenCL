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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 * Copyright 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 * Copyright 1997 Brian Degenhardt <bdegenha@ucsd.edu>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_enum (sampler_type, _("Sampler"), GeglSamplerType, GEGL_TYPE_SAMPLER_TYPE,
                 GEGL_SAMPLER_CUBIC, _("Sampler used internally"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "ditto.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <stdlib.h>

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gint x = result->x; /* initial x                   */
  gint y = result->y; /*           and y coordinates */

  gfloat *dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof (float));
  gfloat *out_pixel = dst_buf;

  GeglSampler *sampler = gegl_buffer_sampler_new (input,
                                                  babl_format ("RGBA float"),
                                                  o->sampler_type);

  gint n_pixels = result->width * result->height;

  while (n_pixels--)
    {
      gegl_sampler_get (sampler,
                        x,
                        y,
                        NULL,
                        out_pixel);

      out_pixel += 4;

      /* update x and y coordinates */
      x++;
      if (x>=result->x + result->width)
        {
          x=result->x;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * 4 * sizeof(gfloat), dst_buf);

  g_object_unref (sampler);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  "categories"  , "distort",
  "name"        , "gegl:ditto",
  "description" , _("Test op to do a 1:1 map of input to output, while sampling"),
  NULL);
}

#endif
