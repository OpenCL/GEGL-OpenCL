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


#ifdef GEGL_PROPERTIES

property_double (value, _("Threshold"), 0.5)
    value_range (-200, 200)
    ui_range    (-1, 2)
    description(_("Scalar threshold level (overridden if an auxiliary input buffer is provided.)."))

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     threshold
#define GEGL_OP_C_SOURCE threshold.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("YA float"));
  gegl_operation_set_format (operation, "aux",    babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  glong   i;

  if (aux == NULL)
    {
      gfloat value = GEGL_PROPERTIES (op)->value;
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

#include "opencl/threshold.cl.h"

static const gchar *composition =
    "<?xml version='1.0'             encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:threshold'>"
    "  <params>"
    "    <param name='value'>0.5</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name" ,       "gegl:threshold",
    "title",       _("Threshold"),
    "categories" , "color",
    "reference-hash", "d20432270a1364932ee88a326a3e26c8",
    "description",
          _("Thresholds the image to white/black based on either the global value "
            "set in the value property, or per pixel from the aux input."),
    "cl-source",   threshold_cl_source,
    "reference-composition", composition,
    NULL);
}

#endif
