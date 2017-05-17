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
 * This operation is a port of the GIMP Apply lens plug-in
 * Copyright (C) 1997 Morten Eriksen mortene@pvv.ntnu.no
 *
 * Porting to GEGL:
 * Copyright 2013 Emanuel Schrade <emanuel.schrade@student.kit.edu>
 * Copyright 2013 Stephan Seifermann <stephan.seifermann@student.kit.edu>
 * Copyright 2013 Bastian Pirk <bastian.pirk@student.kit.edu>
 * Copyright 2013 Pascal Giessler <pascal.giessler@student.kit.edu>
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 */

/* TODO: Find some better algorithm to calculate the roi for each dest
 *       rectangle. Right now it simply asks for the entire image...
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (refraction_index, _("Lens refraction index"), 1.7)
  value_range (1.0, 100.0)
  ui_range    (1.0, 10.0)
  ui_gamma    (3.0)

property_boolean (keep_surroundings, _("Keep original surroundings"), FALSE)
  description(_("Keep image unchanged, where not affected by the lens."))

property_color (background_color, _("Background color"), "none")
  ui_meta ("role", "color-secondary")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     apply_lens
#define GEGL_OP_C_SOURCE apply-lens.c

#include "gegl-op.h"
#include <math.h>

typedef struct
{
  gfloat  bg_color[4];
  gdouble a, b, c;
  gdouble asqr, bsqr, csqr;
} AlParamsType;

/**
 * Computes the original position (ox, oy) of the
 * distorted point (x, y) after passing through the lens
 * which is given by its center and its refraction index.
 * See: Ellipsoid formula: x^2/a^2 + y^2/b^2 + z^2/c^2 = 1.
 */
static void
find_undistorted_pos (gdouble       x,
                      gdouble       y,
                      gdouble       refraction,
                      AlParamsType *params,
                      gdouble      *ox,
                      gdouble      *oy)
{
  gdouble z;
  gdouble nxangle, nyangle, theta1, theta2;
  gdouble ri1 = 1.0;
  gdouble ri2 = refraction;

  z = sqrt ((1 - x * x / params->asqr - y * y / params->bsqr) * params->csqr);

  nxangle = acos (x / sqrt(x * x + z * z));
  theta1 = G_PI / 2.0 - nxangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2.0 - nxangle - theta2;
  *ox = x - tan (theta2) * z;

  nyangle = acos (y / sqrt (y * y + z * z));
  theta1 = G_PI / 2.0 - nyangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2.0 - nyangle - theta2;
  *oy = y - tan (theta2) * z;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = babl_format ("RGBA float");

  GeglRectangle  *whole_region;
  AlParamsType   *params;

  if (! o->user_data)
    o->user_data = g_slice_new0 (AlParamsType);

  params = (AlParamsType *) o->user_data;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  if (whole_region)
    {
      params->a = 0.5 * whole_region->width;
      params->b = 0.5 * whole_region->height;
      params->c = MIN (params->a, params->b);
      params->asqr = params->a * params->a;
      params->bsqr = params->b * params->b;
      params->csqr = params->c * params->c;
    }

  gegl_color_get_pixel (o->background_color, format, params->bg_color);

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
      g_slice_free (AlParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (!in_rect)
    return result;
  else
    return *in_rect;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties       *o = GEGL_PROPERTIES (operation);
  AlParamsType    *params = (AlParamsType *) o->user_data;
  const Babl      *format = babl_format ("RGBA float");

  GeglSampler        *sampler;
  GeglBufferIterator *iter;
  gint                x, y;

  sampler = gegl_buffer_sampler_new_at_level (input, format,
                                              GEGL_SAMPLER_CUBIC, level);

  iter = gegl_buffer_iterator_new (output, roi, level, format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, input, roi, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_pixel = iter->data[0];
      gfloat *in_pixel  = iter->data[1];

      for (y = iter->roi->y; y < iter->roi->y + iter->roi->height; y++)
        {
          gdouble dy, dysqr;

          dy = -((gdouble) y - params->b + 0.5);
          dysqr = dy * dy;

          for (x = iter->roi->x; x < iter->roi->x + iter->roi->width; x++)
            {
              gdouble dx, dxsqr;

              dx = (gdouble) x - params->a + 0.5;
              dxsqr = dx * dx;

              if (dysqr < (params->bsqr - (params->bsqr * dxsqr) / params->asqr))
                {
                  /**
                   * If (x, y) is inside the affected region, we can find its original
                   * position and fetch the pixel with the sampler
                   */
                  gdouble ox, oy;
                  find_undistorted_pos (dx, dy, o->refraction_index, params,
                                        &ox, &oy);

                  gegl_sampler_get (sampler, ox + params->a, params->b - oy,
                                    NULL, out_pixel, GEGL_ABYSS_NONE);
                }
              else
                {
                  /**
                   * Otherwise (that is for pixels outside the lens), we could either leave
                   * the image data unchanged, or set it to a specified 'background_color',
                   * depending on the user input.
                   */
                  if (o->keep_surroundings)
                    memcpy (out_pixel, in_pixel, sizeof (gfloat) * 4);
                  else
                    memcpy (out_pixel, params->bg_color, sizeof (gfloat) * 4);
                }

              out_pixel += 4;
              in_pixel  += 4;
            }
        }
    }

  g_object_unref (sampler);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:apply-lens'>"
    "  <params>"
    "    <param name='refraction_index'>1.7</param>"
    "    <param name='keep_surroundings'>false</param>"
    "    <param name='background_color'>rgba(0, 0.50196, 0.50196, 0.75)</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize                   = finalize;
  operation_class->threaded                = FALSE;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:apply-lens",
    "title",       _("Apply Lens"),
    "categories",  "map",
    "reference-hash", "28c9709b8bac9edf5734dbe45eb31379",
    "license",     "GPL3+",
    "description", _("Simulates the optical distortion caused by having "
                     "an elliptical lens over the image"),
    "reference-composition", composition,
    NULL);
}

#endif
