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
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Lens plug-in - adjust for lens distortion
 * Copyright (C) 2001-2005 David Hodson hodsond@acm.org
 * Many thanks for Lars Clausen for the original inspiration,
 *   useful discussion, optimisation and improvements.
 * Framework borrowed from many similar plugins...
 *
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (main, _("Main:"), -100.0, 100.0, 0.0,
                   _("Main value of distortion"))
gegl_chant_double (zoom, _("Zoom:"), -100.0, 100.0, 0.0,
                   _("Main value of distortion"))
gegl_chant_double (edge, _("Edge:"), -100.0, 100.0, 0.0,
                   _("Edge value of distortion"))
gegl_chant_double (brighten, _("Brighten:"), -100.0, 100.0, 0.0,
                   _("Brighten the image"))
gegl_chant_double (x_shift, _("X shift:"), -100.0, 100.0, 0.0,
                   _("Shift horizontal"))
gegl_chant_double (y_shift, _("Y shift:"), -100.0, 100.0, 0.0,
                   _("Shift vertical"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "lens-distortion.c"

#include "gegl-chant.h"
#include <math.h>

typedef struct
{
  gdouble centre_x;
  gdouble centre_y;
  gdouble mult_sq;
  gdouble mult_qd;
  gdouble rescale;
  gdouble brighten;
  gdouble norm;
} LensDistortion;

static void
lens_setup_calc (GeglChantO     *o,
                 GeglRectangle   boundary,
                 LensDistortion *old)
{
  old->norm = 4.0 / (boundary.width * boundary.width +
                     boundary.height * boundary.height);

  old->centre_x = boundary.width * (100.0 + o->x_shift) / 200.0;
  old->centre_y = boundary.height * (100.0 + o->y_shift) / 200.0;
  old->mult_sq = o->main / 200.0;
  old->mult_qd = o->edge / 200.0;
  old->rescale = pow (2.0, -o->zoom / 100.0);
  old->brighten = -o->brighten / 10.0;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
lens_get_source_coord (gdouble         i,
                       gdouble         j,
                       gdouble        *x,
                       gdouble        *y,
                       gdouble        *mag,
                       LensDistortion *o)
{
  gdouble radius_sq, off_x, off_y, radius_mult;

  off_x = i - o->centre_x;
  off_y = j - o->centre_y;
  radius_sq = (off_x * off_x) + (off_y * off_y);

  radius_sq *= o->norm;

  radius_mult = radius_sq * o->mult_sq + radius_sq * radius_sq * o->mult_qd;

  *mag = radius_mult;
  radius_mult = o->rescale * (1.0 + radius_mult);

  *x = o->centre_x + radius_mult * off_x;
  *y = o->centre_y + radius_mult * off_y;

}

/*
 * Catmull-Rom cubic interpolation
 *
 * equally spaced points p0, p1, p2, p3
 * interpolate 0 <= u < 1 between p1 and p2
 *
 * (1 u u^2 u^3) (  0.0  1.0  0.0  0.0 ) (p0)
 *               ( -0.5  0.0  0.5  0.0 ) (p1)
 *               (  1.0 -2.5  2.0 -0.5 ) (p2)
 *               ( -0.5  1.5 -1.5  0.5 ) (p3)
 *
 */

static void
lens_cubic_interpolate (gfloat  *src,
                        gfloat  *dst,
                        gdouble  dx,
                        gdouble  dy,
                        gdouble  brighten)
{
  gfloat um1, u, up1, up2;
  gfloat vm1, v, vp1, vp2;
  gint   c, row_stride = 16;
  gfloat verts[16];

  um1 = ((-0.5 * dx + 1.0) * dx - 0.5) * dx;
  u = (1.5 * dx - 2.5) * dx * dx + 1.0;
  up1 = ((-1.5 * dx + 2.0) * dx + 0.5) * dx;
  up2 = (0.5 * dx - 0.5) * dx * dx;

  vm1 = ((-0.5 * dy + 1.0) * dy - 0.5) * dy;
  v = (1.5 * dy - 2.5) * dy * dy + 1.0;
  vp1 = ((-1.5 * dy + 2.0) * dy + 0.5) * dy;
  vp2 = (0.5 * dy - 0.5) * dy * dy;

  /* Note: if dst_depth < src_depth, we calculate unneeded pixels here */
  /* later - select or create index array */
  for (c = 0; c < 4 * 4; ++c)
    {
      verts[c] = vm1 * src[c] + v * src[c+row_stride] +
        vp1 * src[c+row_stride*2] + vp2 * src[c+row_stride*3];
    }

  for (c = 0; c < 4; ++c)
    {
      gfloat result;

      result = um1 * verts[c] + u * verts[c+4] +
        up1 * verts[c+4*2] + up2 * verts[c+4*3];

      result *= brighten;

      dst[c] = CLAMP (result, 0.0, 1.0);
    }

}

static void
lens_distort_func (gfloat              *src_buf,
                   gfloat              *dst_buf,
                   const GeglRectangle *extended,
                   const GeglRectangle *result,
                   const GeglRectangle *boundary,
                   LensDistortion       old,
                   gint                 xx,
                   gint                 yy,
                   GeglBuffer          *input)
{
  gdouble sx, sy, mag;
  gdouble brighten;
  gfloat  pixel_buffer [16 * 4], temp[4];
  gdouble dx, dy;
  gint    x_int, y_int, x = 0, y = 0, offset = 0;

  temp[0] = temp[1] = temp[2] = temp[3] = 0.0;

  lens_get_source_coord ((gdouble)xx, (gdouble)yy, &sx, &sy, &mag, &old);

  brighten = 1.0 + mag * old.brighten;

  x_int = floor (sx);
  dx = sx - x_int;

  y_int = floor (sy);
  dy = sy - y_int;

  for (y = y_int - 1; y <= y_int + 2; y++)
    {
      for (x = x_int - 1; x <= x_int + 2; x++)
        {
          gint b;

          if (x >= extended->x && x<(extended->x + extended->width)
              && y >= extended->y && y < (extended->y + extended->height))
            {
              gint src_off;
              src_off = (y - extended->y) * extended->width * 4 +
                (x - extended->x) * 4;
              for (b=0; b<4; b++)
                temp[b] = src_buf[src_off++];

            }
          else if (x >= boundary->x && x < boundary->x + boundary->width &&
                   y >= boundary->y && y < boundary->y + boundary->height)
            {
              gegl_buffer_sample (input, x, y, NULL, temp,
                                  babl_format ("RGBA float"),
                                  GEGL_SAMPLER_CUBIC,
                                  GEGL_ABYSS_NONE);
            }
          else
            {
              for (b=0; b<4; b++)
                temp[b] = 0.0;
            }

          for (b=0; b<4; b++)
            pixel_buffer[offset++] = temp[b];
        }
    }

  lens_cubic_interpolate (pixel_buffer, temp, dx, dy, brighten);

  offset = (yy - result->y) * result->width * 4 + (xx - result->x) * 4;
  for (x=0; x<4; x++)
    dst_buf[offset++] = temp[x];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO          *o = GEGL_CHANT_PROPERTIES (operation);
  LensDistortion       old_lens;
  GeglRectangle        boundary = *gegl_operation_source_get_bounding_box
    (operation, "input");

  gint     x, y;
  gfloat  *src_buf, *dst_buf;

  src_buf    = g_new0 (gfloat, result->width * result->height * 4);
  dst_buf    = g_new0 (gfloat, result->width * result->height * 4);

  lens_setup_calc (o, boundary, &old_lens);

  gegl_buffer_get (input, result, 1.0, babl_format ("RGBA float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y = result->y; y < result->y + result->height; y++)
    for (x = result->x; x < result->x + result->width; x++)
      {
        lens_distort_func (src_buf, dst_buf, result, result, &boundary,
                           old_lens, x, y, input);
      }


  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);
  g_free (src_buf);

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:lens-distortion",
    "categories" , "blur",
    "description", _("Copies image performing lens distortion correction."),
    NULL);
}

#endif
