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
 * Copyright 2006 Geert Jordaens <geert.jordaens@telenet.be>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (values, _("Values"), "", _("list of <number>s"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "svg-luminancetoalpha.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RaGaBaA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat      *in = in_buf;
  gfloat      *out = out_buf;
  gfloat      *m;

  gfloat ma[25] = { 0.0,      0.0,      0.0,     0.0, 0.0,
                    0.0,      0.0,      0.0,     0.0, 0.0,
                    0.0,      0.0,      0.0,     0.0, 0.0,
                    0.2125,   0.7154,   0.0721,  0.0, 0.0,
                    0.0,      0.0,      0.0,     0.0, 1.0};
  glong        i;

  m = ma;
  for (i=0; i<n_pixels; i++)
    {
      out[0] =  m[0]  * in[0] +  m[1]  * in[1] + m[2]  * in[2] + m[3]  * in[3] + m[4];
      out[1] =  m[5]  * in[0] +  m[6]  * in[1] + m[7]  * in[2] + m[8]  * in[3] + m[9];
      out[2] =  m[10] * in[0] +  m[11] * in[1] + m[12] * in[2] + m[13] * in[3] + m[14];
      out[3] =  m[15] * in[0] +  m[16] * in[1] + m[17] * in[2] + m[18] * in[3] + m[19];
      in  += 4;
      out += 4;
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
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:svg-luminancetoalpha",
    "categories" , "compositors:svgfilter",
    "description",
       _("SVG color matrix operation svg_luminancetoalpha"),
        NULL);
}

#endif
