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
 * Author: Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_displace_mode)
  enum_value (GEGL_DISPLACE_MODE_CARTESIAN, "cartesian", N_("Cartesian"))
  enum_value (GEGL_DISPLACE_MODE_POLAR, "polar", N_("Polar"))
enum_end (GeglDisplaceMode)

property_enum (displace_mode, _("Displacement mode"),
                GeglDisplaceMode, gegl_displace_mode,
                GEGL_DISPLACE_MODE_CARTESIAN)
  description (_("Mode of displacement"))

property_enum (sampler_type, _("Sampler"),
               GeglSamplerType, gegl_sampler_type,
               GEGL_SAMPLER_CUBIC)
  description (_("Type of GeglSampler used to fetch input pixels"))

property_enum (abyss_policy, _("Abyss policy"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_CLAMP)
  description (_("How image edges are handled"))

property_double (amount_x, _("X displacement"), 0.0)
    description (_("Displace multiplier for X or radial direction"))
    value_range (-500.0, 500.0)
    ui_range    (-500.0, 500.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_double (amount_y, _("Y displacement"), 0.0)
    description (_("Displace multiplier for Y or tangent (degrees) direction"))
    value_range (-500.0, 500.0)
    ui_range    (-500.0, 500.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE displace.c

#include "gegl-op.h"
#include <math.h>

static gdouble
get_base_displacement (gdouble  amount,
                       gfloat  *map_pixel)
{
  return (2.0 * amount * (map_pixel[0] - 0.5) - 0.5) * map_pixel[1] + 0.5;
}

static inline void
get_input_cartesian_coordinates (gint     x,
                                 gint     y,
                                 gdouble  x_amount,
                                 gdouble  y_amount,
                                 gfloat  *xmap_pixel,
                                 gfloat  *ymap_pixel,
                                 gdouble *x_input,
                                 gdouble *y_input)
{
  *x_input = x;
  *y_input = y;

  if (xmap_pixel && x_amount)
    {
      *x_input += get_base_displacement (x_amount, xmap_pixel);
    }

  if (ymap_pixel && y_amount)
    {
      *y_input += get_base_displacement (y_amount, ymap_pixel);
    }
}

static inline void
get_input_polar_coordinates (gint     x,
                             gint     y,
                             gdouble  x_amount,
                             gdouble  y_amount,
                             gfloat  *xmap_pixel,
                             gfloat  *ymap_pixel,
                             gdouble  cx,
                             gdouble  cy,
                             gdouble *x_input,
                             gdouble *y_input)
{
  gdouble radius, d_alpha;

  radius  = sqrt ((x - cx) * (x - cx) + (y - cy) * (y - cy));
  d_alpha = atan2 (x - cx, y - cy);

  if (xmap_pixel && x_amount)
    {
      radius += get_base_displacement (x_amount, xmap_pixel);
    }

  if (ymap_pixel && y_amount)
    {
      d_alpha += (y_amount / 180) * M_PI * (ymap_pixel[0] - 0.5) / 0.5;
    }

  *x_input = cx + radius * sin (d_alpha);
  *y_input = cy + radius * cos (d_alpha);
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READABLE |
                               GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux",
                               "Aux",
                               "Auxiliary image buffer input pad.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux2",
                               "Aux2",
                               "Second auxiliary image buffer input pad.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}

static void
prepare (GeglOperation *operation)
{
  const Babl *inout_format = babl_format ("R'G'B'A float");
  const Babl *aux_format   = babl_format ("Y'A float");

  gegl_operation_set_format (operation, "input",  inout_format);
  gegl_operation_set_format (operation, "output", inout_format);
  gegl_operation_set_format (operation, "aux",  aux_format);
  gegl_operation_set_format (operation, "aux2", aux_format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle *result = gegl_operation_source_get_bounding_box (operation, "input");

  if (!strcmp (input_pad, "aux")  ||
      !strcmp (input_pad, "aux2") ||
      !result)
    {
      return *roi;
    }

  return *result;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  GeglRectangle *result = gegl_operation_source_get_bounding_box (operation, "input");

  if (!strcmp (input_pad, "aux")  ||
      !strcmp (input_pad, "aux2") ||
      !result)
    {
      return *input_region;
    }

  return *result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *aux2,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o = GEGL_PROPERTIES (operation);
  GeglBufferIterator *iter;
  GeglSampler        *in_sampler;

  gint     x, y;
  gdouble  cx = 0, cy = 0;
  gfloat  *in_pixel;
  gint     n_components;
  gint     aux_index, aux2_index;

  const Babl *inout_format = gegl_operation_get_format (operation, "input");
  const Babl *aux_format  = gegl_operation_get_format (operation, "aux");

  n_components = babl_format_get_n_components (inout_format);

  in_pixel = g_new (gfloat, n_components);

  in_sampler = gegl_buffer_sampler_new_at_level (input, inout_format,
                                                 o->sampler_type, level);

  iter = gegl_buffer_iterator_new (output, result, level, inout_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  if (aux)
    aux_index = gegl_buffer_iterator_add (iter, aux, result, level, aux_format,
                                          GEGL_ACCESS_READ, o->abyss_policy);

  if (aux2)
    aux2_index = gegl_buffer_iterator_add (iter, aux2, result, level, aux_format,
                                           GEGL_ACCESS_READ, o->abyss_policy);

  if (o->displace_mode == GEGL_DISPLACE_MODE_POLAR)
    {
      cx = gegl_buffer_get_width (input) / 2.0;
      cy = gegl_buffer_get_height (input) / 2.0;
    }

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_pixel  = iter->data[0];
      gfloat *aux_pixel  = aux ? iter->data[aux_index] : NULL;
      gfloat *aux2_pixel = aux2 ? iter->data[aux2_index] : NULL;
      gint b;

      for (y = iter->roi[0].y; y < iter->roi[0].y + iter->roi[0].height; y++)
        for (x = iter->roi[0].x; x < iter->roi[0].x + iter->roi[0].width; x++)
          {
            gdouble  src_x, src_y;

            if (o->displace_mode == GEGL_DISPLACE_MODE_POLAR)
              {
                get_input_polar_coordinates (x, y, o->amount_x, o->amount_y,
                                             aux_pixel, aux2_pixel, cx, cy,
                                             &src_x, &src_y);
              }
            else
              {
                get_input_cartesian_coordinates (x, y, o->amount_x, o->amount_y,
                                                 aux_pixel, aux2_pixel,
                                                 &src_x, &src_y);
              }

            gegl_sampler_get (in_sampler, src_x, src_y, NULL,
                              in_pixel, o->abyss_policy);

            for (b = 0; b < n_components; b++)
              out_pixel[b] = in_pixel[b];

            out_pixel += n_components;

            if (aux)
              aux_pixel += 2;

            if (aux2)
              aux2_pixel += 2;
          }
    }

  g_free (in_pixel);
  g_object_unref (in_sampler);

  return  TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer     *input = NULL;
  GeglBuffer     *aux;
  GeglBuffer     *aux2;
  GeglBuffer     *output;
  gboolean        success;

  aux   = gegl_operation_context_get_source (context, "aux");
  aux2  = gegl_operation_context_get_source (context, "aux2");

  if ((!aux && !aux2) ||
      (GEGL_FLOAT_IS_ZERO (o->amount_x) && GEGL_FLOAT_IS_ZERO (o->amount_y)))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      success = TRUE;
    }
  else
    {
      input = gegl_operation_context_get_source (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      success = process (operation, input, aux, aux2, output, result, level);
    }

  if (input != NULL)
    g_object_unref (input);

  if (aux != NULL)
    g_object_unref (aux);

  if (aux2 != NULL)
    g_object_unref (aux2);

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach                    = attach;
  operation_class->prepare                   = prepare;
  operation_class->process                   = operation_process;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->opencl_support            = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:displace",
    "title",       _("Displace"),
    "categories",  "map",
    "license",     "GPL3+",
    "description", _("Displace pixels as indicated by displacement maps"),
    NULL);
}

#endif
