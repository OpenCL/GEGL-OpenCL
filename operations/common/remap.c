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
 * Copyright 2006-2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

/* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER3
#define GEGL_CHANT_C_FILE        "remap.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "aux2",   format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation        *op,
          void                *in_buf,
          void                *min_buf,
          void                *max_buf,
          void                *out_buf,
          glong                n_pixels,
          const GeglRectangle *roi,
          gint                 level)
{
  gint i;
  gfloat *in = in_buf;
  gfloat *min = min_buf;
  gfloat *max = max_buf;
  gfloat *out = out_buf;

  for (i = 0; i < n_pixels; i++)
    {
       gint c;
       for (c = 0; c < 3; c++)
         {
           gfloat delta = max[c]-min[c];

           if (delta > 0.0001 || delta < -0.0001)
             out[c] = (in[c]-min[c]) / delta;
           else
             out[c] = in[c];
           out[3] = in[3];
         }
      in  += 4;
      out += 4;
      min += 4;
      max += 4;
    }
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_composer3_class;

  operation_class       = GEGL_OPERATION_CLASS (klass);
  point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);
  operation_class->prepare = prepare;

  point_composer3_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:remap",
    "description",
          _("stretch components of pixels individually based on luminance envelopes"),
    "categories" , "compositors:porter-duff",
    NULL);
}

#endif
