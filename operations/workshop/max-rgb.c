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
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.org>
 *
 * Based on "Maximum RGB" GIMP plugin
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 * Copyright (C) 2000 tim copperfield <timecop@japan.co.jp>
 *
 * The common/brightness-contrast.c operation by Øyvind Kolås was used
 * as a template for this op file.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_boolean (min,   _("Minimal"),   FALSE,
                    _("Hold the minimal values instead of the maximal values"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "max-rgb.c"

#include "gegl-chant.h"


static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     * GEGL_ALIGNED in_pixel;
  gfloat     * GEGL_ALIGNED out_pixel;
  gfloat      val;
  glong       i;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  /* The code could be compacted by inserting the if into the loop, but
   * it's more efficient if we do it like this. Note also that in cases
   * of equality between the maximal/minimal channels, we want to keep
   * them all.
   */
  if (!o->min)
    {
      for (i=0; i<n_pixels; i++)
        {
          val = MAX (in_pixel[0], MAX (in_pixel[1], in_pixel[2]));

          out_pixel[0] = (in_pixel[0] == val) ? val : 0;
          out_pixel[1] = (in_pixel[1] == val) ? val : 0;
          out_pixel[2] = (in_pixel[2] == val) ? val : 0;

          out_pixel[3] = in_pixel[3]; /* copy the alpha */

          in_pixel  += 4;
          out_pixel += 4;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          val = MIN (in_pixel[0], MIN (in_pixel[1], in_pixel[2]));

          out_pixel[0] = (in_pixel[0] == val) ? val : 0;
          out_pixel[1] = (in_pixel[1] == val) ? val : 0;
          out_pixel[2] = (in_pixel[2] == val) ? val : 0;

          out_pixel[3] = in_pixel[3]; /* copy the alpha */

          in_pixel  += 4;
          out_pixel += 4;
        }
    }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:max-rgb",
    "categories"  , "color",
    "description" , _("Reduce image to pure red, green, and blue"),
    NULL);
}

#endif
