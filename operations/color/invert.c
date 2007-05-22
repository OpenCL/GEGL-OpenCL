/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
   /* no properties */
#else

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_NAME          invert
#define GEGL_CHANT_DESCRIPTION   "Inverts the components (except alpha), the result is the corresponding \"negative\" image."
#define GEGL_CHANT_SELF          "invert.c"
#define GEGL_CHANT_CATEGORIES    "color"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *op,
         void          *in_buf,
         void          *out_buf,
         glong          samples) 
{
  gint i;
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

#endif
