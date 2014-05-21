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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (refraction_index, _("Lens refraction index"), 1.7)
  value_range (1.0, 100.0)

property_boolean (keep_surroundings, _("Keep original surroundings"), FALSE)
  description(_("Keep image unchanged, where not affected by the lens."))

property_color (background_color, _("Background color"), "none")
  //ui_meta ("role", "color-secondary")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_FILE "apply-lens.c"

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

/**
 * Computes the projected position (projx, projy) of the
 * original point (x, y) after passing through the lens
 * which is given by its center and its refraction index.
 * See: Ellipsoid formula: x^2/a^2 + y^2/b^2 + z^2/c^2 = 1.
 *
 * @param a2 semiaxis a
 * @param b2 semiaxis b
 * @param c2 semiaxis c
 * @param x coordinate x
 * @param y coordinate y
 * @param refraction refraction index
 * @param projy inout of the projection x
 * @param projy inout of the projection y
 */
static void
find_projected_pos (gfloat  a2,
                    gfloat  b2,
                    gfloat  c2,
                    gfloat  x,
                    gfloat  y,
                    gfloat  refraction,
                    gfloat *projx,
                    gfloat *projy)
{
  gfloat z;
  gfloat nxangle, nyangle, theta1, theta2;
  gfloat ri1 = 1.0;
  gfloat ri2 = refraction;

  z = sqrt ((1 - x * x / a2 - y * y / b2) * c2);

  nxangle = acos (x / sqrt(x * x + z * z));
  theta1 = G_PI / 2 - nxangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2 - nxangle - theta2;
  *projx = x - tan (theta2) * z;

  nyangle = acos (y / sqrt (y * y + z * z));
  theta1 = G_PI / 2 - nyangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2 - nyangle - theta2;
  *projy = y - tan (theta2) * z;
}

/**
 * Prepare function of gegl filter.
 * @param operation given Gegl operation
 */
static void
prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/**
 * Returns the cached region. This is an area filter, which acts on the whole image.
 * @param operation given Gegl operation
 * @param roi the rectangle of interest
 * @return result the new rectangle
 */
static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

/**
 * Process the gegl filter
 * @param operation the given Gegl operation
 * @param input the input buffer.
 * @param output the output buffer.
 * @param result the region of interest.
 * @param level the level of detail
 * @return True, if the filter was successfull applied.
 */
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o = GEGL_PROPERTIES (operation);
  const Babl  *input_format = gegl_buffer_get_format (input);
  const int bytes_per_pixel = babl_format_get_bytes_per_pixel (input_format);

  /**
   * src_buf, dst_buf:
   * Input- and output-buffers, containing the whole selection,
   * the filter should be applied on.
   *
   * src_pixel, dst_pixel:
   * pointers to the current source- and destination-pixel.
   * Using these pointers, the reading and writing can be delayed till the
   * end of the loop. Hence src_pixel can also point to background_color if
   * necessary.
   */
  guint8    *src_buf, *dst_buf, *src_pixel, *dst_pixel, background_color[bytes_per_pixel];

  /**
   * (x, y): Position of the pixel we compute at the moment.
   * (width, height): Dimensions of the lens
   * pixel_offset: For calculating the pixels position in src_buf / dst_buf.
   */
  gint         x, y,
               width, height,
               pixel_offset, projected_pixel_offset;

  /**
   * Further parameters that are needed to calculate the position of a projected
   * further pixel at (x, y).
   */
  gfloat       a, b, c,
               asqr, bsqr, csqr,
               dx, dy, dysqr,
               projected_x, projected_y,
               refraction_index;

  gboolean     keep_surroundings;

  width = result->width;
  height = result->height;

  refraction_index = o->refraction_index;
  keep_surroundings = o->keep_surroundings;
  gegl_color_get_pixel (o->background_color, input_format, background_color);

  a = 0.5 * width;
  b = 0.5 * height;
  c = MIN (a, b);
  asqr = a * a;
  bsqr = b * b;
  csqr = c * c;

  /**
   * Todo: We might want to change the buffers sizes, as the memory consumption
   * could be rather large.However, due to the lens, it might happen, that one pixel from the
   * images center gets stretched to the whole image, so we will still need
   * at least a src_buf of size (width/2) * (height/2) * 4 to be able to
   * process one quarter of the image.
   */
  src_buf = gegl_malloc (width * height * bytes_per_pixel);
  dst_buf = gegl_malloc (width * height * bytes_per_pixel);

  gegl_buffer_get (input, result, 1.0, input_format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y = 0; y < height; y++)
    {
      /* dy is the current pixels distance (in y-direction) of the lenses center */
      dy = -((gfloat)y - b + 0.5);

      /**
       * To determine, whether the pixel is within the elliptical region affected
       * by the lens, we furthermore need the squared distance. So we calculate it
       * once for each row.
       */
      dysqr = dy * dy;
      for (x = 0; x < width; x++)
        {
          /* Again we need to calculate the pixels distance from the lens dx. */
          dx = (gfloat)x - a + 0.5;

          /* Given x and y we can now determine the pixels offset in our buffer. */
          pixel_offset = (x + y * width) * bytes_per_pixel;

          /* As described above, we only read and write image data once. */
          dst_pixel = &dst_buf[pixel_offset];

          if (dysqr < (bsqr - (bsqr * dx * dx) / asqr))
            {
              /**
               * If (x, y) is inside the affected region, we can find its projected
               * position, calculate the projected_pixel_offset and set the src_pixel
               * to where we want to copy the pixel-data from.
               */
              find_projected_pos (asqr, bsqr, csqr, dx, dy, refraction_index,
                                  &projected_x, &projected_y);

              projected_pixel_offset = ( (gint)(-projected_y + b) * width +
                                         (gint)( projected_x + a) ) * bytes_per_pixel;

              src_pixel = &src_buf[projected_pixel_offset];
            }
          else
            {
              /**
               * Otherwise (that is for pixels outside the lens), we could either leave
               * the image data unchanged, or set it to a specified 'background_color',
               * depending on the user input.
               */
              if (keep_surroundings)
                src_pixel = &src_buf[pixel_offset];
              else
                src_pixel = background_color;
            }

          /**
           * At the end, we can copy the src_pixel (which was determined above), to
           * dst_pixel.
           */
          memcpy (dst_pixel, src_pixel, bytes_per_pixel);
        }
  }

  gegl_buffer_set (output, result, 1.0, gegl_buffer_get_format (output), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);

  gegl_free (dst_buf);
  gegl_free (src_buf);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
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

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare                 = prepare;
  operation_class->get_cached_region       = get_cached_region;
  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:apply-lens",
    "categories",  "Distorts",
    "license",     "GPL3+",
    "description", _("Simulate an elliptical lens over the image"),
    "reference-composition", composition,
    NULL);
}

#endif
