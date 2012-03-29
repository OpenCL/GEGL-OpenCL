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

#ifdef GEGL_CHANT_PROPERTIES

   /* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "unpremul.c"
#define GEGLV4

#include "gegl-chant.h"

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  glong   i;
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  for (i=0; i<samples; i++)
    {
      int  j;
      for (j=0; j<3; j++)
        {
          if (in[3] != 0)
            out[j] = in[j] / in[3];
          else
            out[j] = 0.0;
        }
      out[3]=in[3];
      in += 4;
      out+= 4;
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

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:unpremul",
    "categories"  , "color",
    "description" , "Unpremultiplies a buffer that contains pre-multiplied colors (but is marked as not having it)",
    NULL);
}

#endif
