/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 *
 * Author: robert@experimental.net
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_color (from_color, _("From Color"), "white")
    description(_("The color to change."))

property_double (red_threshold, _("Red Threshold"), 0.0)
    description (_("Red threshold of the input color"))
    value_range (0.0, 1.0)

property_double (green_threshold, _("Green Threshold"), 0.0)
    description (_("Green threshold of the input color"))
    value_range (0.0, 1.0)

property_double (blue_threshold, _("Blue Threshold"), 0.0)
    description (_("Blue threshold of the input color"))
    value_range (0.0, 1.0)

property_color (to_color, _("To Color"), "black")
    description(_("Replacement color."))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_FILE "color-exchange.c"

#include "gegl-op.h"

typedef struct
{
  gfloat color_in[3];
  gfloat color_out[3];
  gfloat min[3];
  gfloat max[3];
} CeParamsType;

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("R'G'B'A float");
  const Babl     *colorformat = babl_format ("R'G'B' float");
  CeParamsType *params;

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (CeParamsType);

  params = (CeParamsType*) o->user_data;

  gegl_color_get_pixel (o->from_color, colorformat, &(params->color_in));
  gegl_color_get_pixel (o->to_color, colorformat, &(params->color_out));

  params->min[0] = CLAMP (params->color_in[0] - o->red_threshold,
                          0.0, 1.0);
  params->max[0] = CLAMP (params->color_in[0] + o->red_threshold,
                          0.0, 1.0);
  params->min[1] = CLAMP (params->color_in[1] - o->green_threshold,
                          0.0, 1.0);
  params->max[1] = CLAMP (params->color_in[1] + o->green_threshold,
                          0.0, 1.0);
  params->min[2] = CLAMP (params->color_in[2] - o->blue_threshold,
                          0.0, 1.0);
  params->max[2] = CLAMP (params->color_in[2] + o->blue_threshold,
                          0.0, 1.0);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (CeParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties     *o      = GEGL_PROPERTIES (operation);
  const CeParamsType *params = (CeParamsType*) o->user_data;
  gint    chan;
  gfloat *input  = in_buf;
  gfloat *output = out_buf;

  g_assert (params != NULL);

  while (n_pixels--)
    {
      if ((GEGL_FLOAT_EQUAL (input[0], params->min[0]) ||
          (input[0] > params->min[0])) &&
          (GEGL_FLOAT_EQUAL (input[0], params->max[0]) ||
          (input[0] < params->max[0])) &&
          (GEGL_FLOAT_EQUAL (input[1], params->min[1]) ||
          (input[1] > params->min[1])) &&
          (GEGL_FLOAT_EQUAL (input[1], params->max[1]) ||
          (input[1] < params->max[1])) &&
          (GEGL_FLOAT_EQUAL (input[2], params->min[2]) ||
          (input[2] > params->min[2])) &&
          (GEGL_FLOAT_EQUAL (input[2], params->max[2]) ||
          (input[2] < params->max[2])))
        {
          gfloat delta[3];

          for (chan = 0; chan < 3 ; chan++)
            {
              delta[chan] = input[chan] > params->color_in[chan] ?
                input[chan] - params->color_in[chan] :
                params->color_in[chan] - input[chan];

              output[chan] = CLAMP (params->color_out[chan] + delta[chan],
                                    0.0, 1.0);
            }
        }
      else
        {
          output[0] = input[0];
          output[1] = input[1];
          output[2] = input[2];
        }

      output[3] = input[3];

      input  += 4;
      output += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-exchange",
    "title",       _("Exchange color"),
    "categories",  "color",
    "license",     "GPL3+",
    "description", _("Exchange one color with another, optionally setting "
                     "a threshold to convert from one shade to another."),
    NULL);
}

#endif
