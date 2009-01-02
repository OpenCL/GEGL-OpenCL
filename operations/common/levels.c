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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (in_low, _("Low input"), -1.0, 4.0, 0.0,
                   _("Input luminance level to become lowest output"))
gegl_chant_double (in_high, _("High input"), -1.0, 4.0, 1.0,
                   _("Input luminance level to become white."))
gegl_chant_double (out_low, _("Low output"), -1.0, 4.0, 0.0,
                   _("Lowest luminance level in output"))
gegl_chant_double (out_high, _("High output"),
                   -1.0, 4.0, 1.0, _("Highest luminance level in output"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE         "levels.c"

#include "gegl-chant.h"

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      in_range;
  gfloat      out_range;
  gfloat      in_offset;
  gfloat      out_offset;
  gfloat      scale;
  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;

  in_offset = o->in_low * 1.0;
  out_offset = o->out_low * 1.0;
  in_range = o->in_high-o->in_low;
  out_range = o->out_high-o->out_low;

  if (in_range == 0.0)
    in_range = 0.00000001;

  scale = out_range/in_range;

  for (i=0; i<n_pixels; i++)
    {
      int c;
      for (c=0;c<3;c++)
        out_pixel[c] = (in_pixel[c]- in_offset) * scale + out_offset;
      out_pixel[3] = in_pixel[3];
      out_pixel += 4;
      in_pixel += 4;
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

  point_filter_class->process = process;

  operation_class->name        = "gegl:levels";
  operation_class->categories  = "color";
  operation_class->description =
        _("Remaps the intensity range of the image");
}

#endif
