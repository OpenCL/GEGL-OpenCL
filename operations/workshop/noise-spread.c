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
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.org>
 *
 * Based on "Spread" (Noise) GIMP plugin
 * Copyright (C) 1997 Brian Degenhardt and Federico Mena Quintero
 *
 * The workshop/whirl-pinch.c and common/pixelise.c were used as
 * templates for this op file.
 */
#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (x_amount, _("Horizontal"), 0, 256, 5,
   _("Horizontal spread amount"))
gegl_chant_int (y_amount, _("Vertical"), 0, 256, 5,
   _("Vertical spread amount"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "noise-spread.c"

#include "gegl-chant.h"
#include <math.h>

static void
calc_sample_coords (gint      src_x,
                    gint      src_y,
                    gint      x_amount,
                    gint      y_amount,
                    GRand    *gr,
                    gdouble  *x,
                    gdouble  *y)
{
  gdouble        angle;
  gint           xdist, ydist;

  /* get random angle, x distance, and y distance */
  xdist = (x_amount > 0
           ? g_rand_int_range (gr, -x_amount, x_amount)
           : 0);
  ydist = (y_amount > 0
           ? g_rand_int_range (gr, -y_amount, y_amount)
           : 0);
  angle = g_rand_double_range (gr, -G_PI, G_PI);

  *x = src_x + floor (sin (angle) * xdist);
  *y = src_y + floor (cos (angle) * ydist);
}

/* Apply the actual transform */
static void
apply_spread (gint                 x_amount,
              gint                 y_amount,
              gint                 img_width,
              gint                 img_height,
              const Babl          *format,
              GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *roi)
{
  gfloat *dst_buf;
  gint x1, y1; // Noise image
  gdouble x, y;   // Source Image

  GRand    *gr = g_rand_new ();

  /* Get buffer in which to place dst pixels. */
  dst_buf = g_new0 (gfloat, roi->width * roi->height * 4);

  for (y1 = 0; y1 < roi->height; y1++) {
    for (x1 = 0; x1 < roi->width; x1++) {

      calc_sample_coords (x1, y1, x_amount, y_amount, gr, &x, &y);
      /* Only displace the pixel if it's within the bounds of the image. */
      if (x >= 0 && x < img_width && y >= 0 && y < img_height)
        gegl_buffer_sample (src, x, y, NULL, &dst_buf[(y1 * roi->width + x1) * 4], format,
                            GEGL_SAMPLER_LINEAR, GEGL_ABYSS_NONE);
      else /* Else just copy it */
        gegl_buffer_sample (src, x1, y1, NULL, &dst_buf[(y1 * roi->width + x1) * 4], format,
                            GEGL_SAMPLER_LINEAR, GEGL_ABYSS_NONE);
    } /* for */
  } /* for */

  gegl_buffer_sample_cleanup (src);

  /* Store dst pixels. */
  gegl_buffer_set (dst, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  gegl_buffer_flush(dst);

  g_free (dst_buf);
  g_rand_free (gr);
}

/*****************************************************************************/

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  = o->x_amount;
  op_area->top    =
  op_area->bottom = o->y_amount;

  gegl_operation_set_format (operation, "output",
                             babl_format ("RaGaBaA float"));
}

/* Perform the specified operation.
 */
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle boundary = gegl_operation_get_bounding_box (operation);
  const Babl *format = babl_format ("RaGaBaA float");

  apply_spread ((o->x_amount + 1) / 2,
                (o->y_amount + 1) / 2,
                boundary.width - 2 * o->x_amount,
                boundary.height - 2 * o->y_amount,
                format,
                input,
                output,
                result);
  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "categories" , "noise",
    "name"       , "gegl:noise-spread",
    "description", _("Noise spread filter"),
    NULL);
}


#endif
