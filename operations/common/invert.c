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

   /* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "invert.c"
#define GEGLV4

#include "gegl-chant.h"

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi)
{
  glong   i;
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  for (i=0; i<samples; i++)
    {
      int  j;
      for (j=0; j<3; j++)
        {
          gfloat c;
          c = in[j];
          c = 1.0 - c;
          out[j] = c;
        }
      out[3]=in[3];
      in += 4;
      out+= 4;
    }
  return TRUE;
}

#ifdef HAS_G4FLOAT
static gboolean
process_simd (GeglOperation       *op,
              void                *in_buf,
              void                *out_buf,
              glong                samples,
              const GeglRectangle *roi)
{
  g4float *in  = in_buf;
  g4float *out = out_buf;
  g4float  one = g4float_one;

  while (samples--)
    {
      gfloat a= g4float_a(*in)[3];
      *out = one - *in;
      g4float_a(*out)[3]=a;
      in  ++;
      out ++;
    }
  return TRUE;
}
#endif

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;

  operation_class->name        = "gegl:invert";
  operation_class->categories  = "color";
  operation_class->description =
     _("Inverts the components (except alpha), the result is the "
       "corresponding \"negative\" image.");

#ifdef HAS_G4FLOAT
  gegl_operation_class_add_processor (operation_class,
                                      G_CALLBACK (process_simd), "simd");
#endif
}

#endif
