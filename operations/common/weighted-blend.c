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
 *           2008 James Legg
 */
#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (value, _("Value"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, _("global value used if aux doesn't contain data"))

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE       "weighted-blend.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gint    i;

  if (aux == NULL)
    {
      /* there is no auxilary buffer.
       * output the input buffer.
       * gfloat value = GEGL_CHANT_PROPERTIES (op)->value;
       */
      for (i = 0; i < n_pixels; i++)
        {
          gint j;
          for (j = 0; j < 4; j++)
            {
              out[j] = in[j];
            }
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          gfloat total_alpha;
          /* find the proportion between alpha values */
          total_alpha = in[3] + aux[3];
          if (!total_alpha)
            {
              /* no coverage from any source pixel */
              for (j = 0; j < 4; j++)
                {
                  out[j] = 0.0;
                }
            }
          else
            {
              /* the total alpha is non-zero, therefore we may find a colour from a weighted blend */
              gfloat in_weight = in[3] / total_alpha;
              gfloat aux_weight = 1.0 - in_weight;
              for (j = 0; j < 3; j++)
                {
                  out[j] = in_weight * in[j] + aux_weight * aux[j];
                }
              out[3] = total_alpha;
            }
          in  += 4;
          aux += 4;
          out += 4;
        }
    }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare      = prepare;

  operation_class->name        = "gegl:weighted-blend";
  operation_class->categories  = "compositors:blend";
  operation_class->description =
    _("blend two images using alpha values as weights");
}
#endif
