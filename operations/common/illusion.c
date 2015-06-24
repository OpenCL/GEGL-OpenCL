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

enum_start (gegl_illusion_type)
  enum_value (GEGL_ILLUSION_TYPE_1, "type1", N_("Type 1"))
  enum_value (GEGL_ILLUSION_TYPE_2, "type2", N_("Type 2"))
enum_end (GeglIllusionType)

property_int  (division, _("Division"), 8)
  description (_("The number of divisions"))
  value_range (0, 64)
  ui_range    (0, 64)

property_enum (illusion_type, _("Illusion type"),
                GeglIllusionType, gegl_illusion_type,
                GEGL_ILLUSION_TYPE_1)
  description (_("Type of illusion"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE illusion.c

#include "gegl-op.h"
#include <math.h>

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  if (! format || ! babl_format_has_alpha (format))
    format = babl_format ("R'G'B' float");
  else
    format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o = GEGL_PROPERTIES (operation);
  GeglBufferIterator *iter;
  GeglSampler        *sampler;

  gint         x, y, xx, yy, b;
  gint         width, height, components;
  gdouble      radius, cx, cy, angle;
  gdouble      center_x;
  gdouble      center_y;
  gdouble      scale;
  gdouble      offset;
  gboolean     has_alpha;
  gfloat       alpha, alpha1, alpha2;
  gfloat      *in_pixel1;
  gfloat      *in_pixel2;

  const Babl *format = gegl_operation_get_format (operation, "output");
  has_alpha  = babl_format_has_alpha (format);

  if (has_alpha)
    components = 4;
  else
    components = 3;

  in_pixel1 = g_new (float, components);
  in_pixel2 = g_new (float, components);

  iter = gegl_buffer_iterator_new (output, result, level, format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  sampler = gegl_buffer_sampler_new_at_level (input, format,
                                              GEGL_SAMPLER_NEAREST, level);

  width = result->width;
  height = result->height;

  center_x = width / 2.0;
  center_y = height / 2.0;
  scale = sqrt (width * width + height * height) / 2;
  offset = (gint) (scale / 2);

  while (gegl_buffer_iterator_next (iter))
    {
       gfloat  *out_pixel = iter->data[0];

       for (y = iter->roi[0].y; y < iter->roi[0].y + iter->roi[0].height; ++y)
         for (x = iter->roi[0].x; x < iter->roi[0].x + iter->roi[0].width; ++x)
           {
              cy = ((gdouble) y - center_y) / scale;
              cx = ((gdouble) x - center_x) / scale;

              angle = floor (atan2 (cy, cx) * o->division / G_PI_2) *
                             G_PI_2 / o->division + (G_PI / o->division);
              radius = sqrt ((gdouble) (cx * cx + cy * cy));

              if (o->illusion_type == GEGL_ILLUSION_TYPE_1)
                {
                   xx = x - offset * cos (angle);
                   yy = y - offset * sin (angle);
                }
              else                          /* GEGL_ILLUSION_TYPE_2 */
                {
                   xx = x - offset * sin (angle);
                   yy = y - offset * cos (angle);
                }

                gegl_sampler_get (sampler, x, y, NULL,
                                  in_pixel1, GEGL_ABYSS_CLAMP);

                gegl_sampler_get (sampler, xx, yy, NULL,
                                  in_pixel2, GEGL_ABYSS_CLAMP);

                if (has_alpha)
                  {
                     alpha1 = in_pixel1[3];
                     alpha2 = in_pixel2[3];
                     alpha  = (1 - radius) * alpha1 + radius * alpha2;

                     if ((out_pixel[3] = (alpha / 2)))
                       {
                         for (b = 0; b < 3; b++)
                           out_pixel[b] = ((1 - radius) * in_pixel1[b] * alpha1 +
                                           radius * in_pixel2[b] * alpha2) / alpha;
                       }
                  }
                else
                 {
                   for (b = 0; b < 3; b++)
                     out_pixel[b] = (1 - radius) * in_pixel1[b] + radius * in_pixel2[b];
                 }

                 out_pixel += components;
           }
    }

  g_free (in_pixel1);
  g_free (in_pixel2);

  g_object_unref (sampler);

  return  TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:illusion",
      "title",       _("Illusion"),
      "categories",  "map",
      "license",     "GPL3+",
      "description", _("Superimpose many altered copies of the image."),
      NULL);
}

#endif
