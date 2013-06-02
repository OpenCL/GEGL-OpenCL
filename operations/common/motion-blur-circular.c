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
 * Copyright 2013 Téo Mazars <teo.mazars@ensimag.fr>
 *
 * This operation is inspired by and uses parts of blur-motion.c
 * from GIMP 2.8.4.
 *
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (center_x, _("X"),
                      -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                      -100000.0, 100000.0, 1.0,
                      _("Horizontal center position"))

gegl_chant_double_ui (center_y, _("Y"),
                      -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                      -100000.0, 100000.0, 1.0,
                      _("Vertical center position"))

gegl_chant_double_ui (angle, _("angle"),
                      0.0, 1.0, 0.02,
                      0.0, 1.0, 2.0,
                      _("Rotation blur angle"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "motion-blur-circular.c"

#include "gegl-chant.h"


#define SQR(c)  ((c) * (c))

#define MAX_NUM_IT      200
#define NOMINAL_NUM_IT  100
#define SQRT_2          1.41


static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter* op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO* o = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle *whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  gdouble angle  = o->angle * G_PI;

  if (whole_region != NULL)
    {
      gdouble maxr_x = MAX (fabs (o->center_x - whole_region->x),
                            fabs (o->center_x - whole_region->x - whole_region->width));
      gdouble maxr_y = MAX (fabs (o->center_y - whole_region->y),
                            fabs (o->center_y - whole_region->y - whole_region->height));

      op_area->left = op_area->right
        = ceil (maxr_y * sin (angle / 2.0)) + 1;

      op_area->top = op_area->bottom
        = ceil (maxr_x * sin (angle / 2.0)) + 1;
    }
  else
    {
      op_area->left   =
      op_area->right  =
      op_area->top    =
      op_area->bottom = 0;
    }

  gegl_operation_set_format (operation, "input",  babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}


static inline gfloat*
get_pixel_color (gfloat *in_buf,
                 const GeglRectangle *rect,
                 gint x,
                 gint y)
{
  gint ix = x - rect->x;
  gint iy = y - rect->y;
  ix = CLAMP (ix, 0, rect->width  - 1);
  iy = CLAMP (iy, 0, rect->height - 1);

  return &in_buf[(iy * rect->width + ix) * 4];
}


static inline gdouble
compute_phi (gdouble xr, gdouble yr)
{
  gdouble phi;
  if (fabs (xr) > 0.00001)
    {
      phi = atan (yr / xr);
      if (xr < 0.0)
        phi = G_PI + phi;
    }
  else
    {
      if (yr >= 0.0)
        phi = G_PI_2;
      else
        phi = -G_PI_2;
    }
  return phi;
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o;
  GeglOperationAreaFilter *op_area;
  gfloat *in_buf, *out_buf, *out_pixel;
  gint x, y;
  GeglRectangle src_rect;
  gdouble angle;

  GeglRectangle *whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o = GEGL_CHANT_PROPERTIES (operation);

  src_rect = *roi;
  src_rect.x -= op_area->left;
  src_rect.y -= op_area->top;
  src_rect.width += op_area->left + op_area->right;
  src_rect.height += op_area->top + op_area->bottom;

  in_buf    = g_new  (gfloat, src_rect.width * src_rect.height * 4);
  out_buf   = g_new0 (gfloat, roi->width * roi->height * 4);
  out_pixel = out_buf;

  gegl_buffer_get (input,
                   &src_rect,
                   1.0,
                   babl_format ("RaGaBaA float"),
                   in_buf,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  angle = o->angle * G_PI;

  for (y = roi->y; y < roi->height + roi->y; ++y)
    {
      for (x = roi->x; x < roi->width + roi->x; ++x)
        {
          gint c, i;
          gdouble phi_base, phi_start, phi_step;
          gfloat sum[] = {0, 0, 0, 0};
          gint count = 0;

          gdouble xr = x - o->center_x;
          gdouble yr = y - o->center_y;
          gdouble radius  = sqrt (SQR (xr) + SQR (yr));

          /* This is not the "real" length, a bit shorter */
          gdouble arc_length = radius * angle * SQRT_2;

          /* ensure quality with small angles */
          gint n = MAX (ceil (arc_length), 3);

          /* performance concern */
          if (n > NOMINAL_NUM_IT)
            n = MIN (NOMINAL_NUM_IT + (gint) sqrt (n - NOMINAL_NUM_IT), MAX_NUM_IT);

          phi_base = compute_phi (xr, yr);
          phi_start = phi_base + angle / 2.0;
          phi_step = angle / (gdouble) n;

          /* Iterate other the arc */
          for (i = 0; i < n; i++)
            {
              gfloat s_val = sin (phi_start - i * phi_step);
              gfloat c_val = cos (phi_start - i * phi_step);

              gfloat ix = o->center_x + radius * c_val;
              gfloat iy = o->center_y + radius * s_val;

              if (ix >= whole_region->x && ix < whole_region->x + whole_region->width &&
                  iy >= whole_region->y && iy < whole_region->y + whole_region->height)
                {
                  /* do bilinear interpolation to get a nice smooth result */
                  gfloat dx = ix - floor (ix);
                  gfloat dy = iy - floor (iy);

                  gfloat *pix0, *pix1, *pix2, *pix3;
                  gfloat mixy0[4];
                  gfloat mixy1[4];

                  pix0 = get_pixel_color (in_buf, &src_rect, ix, iy);
                  pix1 = get_pixel_color (in_buf, &src_rect, ix+1, iy);
                  pix2 = get_pixel_color (in_buf, &src_rect, ix, iy+1);
                  pix3 = get_pixel_color (in_buf, &src_rect, ix+1, iy+1);

                  for (c = 0; c < 4; ++c)
                    {
                      mixy0[c] = dy * (pix2[c] - pix0[c]) + pix0[c];
                      mixy1[c] = dy * (pix3[c] - pix1[c]) + pix1[c];

                      sum[c] +=  dx * (mixy1[c] - mixy0[c]) + mixy0[c];
                    }

                  count++;
                }
            }

          if (count == 0)
            {
              gfloat *pix = get_pixel_color (in_buf, &src_rect, x, y);
              for (c = 0; c < 4; ++c)
                *out_pixel++ = pix[c];
            }
          else
            {
              for (c = 0; c < 4; ++c)
                *out_pixel++ = sum[c] / (gfloat) count;
            }
        }
    }

  gegl_buffer_set (output,
                   roi,
                   1.0,
                   babl_format ("RaGaBaA float"),
                   out_buf,
                   GEGL_AUTO_ROWSTRIDE);

  g_free (in_buf);
  g_free (out_buf);

  return  TRUE;
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
                                 "name"       , "gegl:motion-blur-circular",
                                 "categories" , "blur",
                                 "description", _("Circular motion blur"),
                                 NULL);
}

#endif
