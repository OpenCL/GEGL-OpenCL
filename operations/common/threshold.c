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

gegl_chant_double (value, _("Threshold"), -10.0, 10.0, 0.5,
   _("Global threshold level (used when there is no auxiliary input buffer)."))

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE       "threshold.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("YA float"));
  gegl_operation_set_format (operation, "aux", babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  glong   i;

  if (aux == NULL)
    {
      gfloat value = GEGL_CHANT_PROPERTIES (op)->value;
      for (i=0; i<n_pixels; i++)
        {
          gfloat c;

          c = in[0];
          c = c>=value?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gfloat value = *aux;
          gfloat c;

          c = in[0];
          c = c>=value?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
          aux += 1;
        }
    }
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:threshold";
  operation_class->categories  = "color";
  operation_class->description =
        _("Thresholds the image to white/black based on either the global value "
          "set in the value property, or per pixel from the aux input.");
}

#endif
