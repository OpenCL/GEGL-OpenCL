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
 * Lens plug-in - adjust for lens distortion
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2001-2005 David Hodson <hodsond@acm.org>
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright (C) 2013 Téo Mazars <teo.mazars@ensimag.fr>
 *
 * Many thanks for Lars Clausen for the original inspiration,
 *   useful discussion, optimisation and improvements.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (main, _("Main"), 0.0)
    description (_("Amount of second-order distortion"))
    value_range (-100.0, 100.0)

property_double (edge, _("Edge"), 0.0)
    description (_("Amount of fourth-order distortion"))
    value_range (-100.0, 100.0)

property_double (zoom, _("Zoom"), 0.0)
    description (_("Rescale overall image size"))
    value_range (-100, 100.0)

property_double (x_shift, _("Shift X"), 0.0)
    description (_("Effect centre offset in X"))
    value_range (-100.0, 100.0)

property_double (y_shift, _("Shift Y"), 0.0)
    description (_("Effect centre offset in Y"))
    value_range (-100.0, 100.0)

property_double (brighten, _("Brighten"), 0.0)
    description (_("Adjust brightness in corners"))
    value_range (-100.0, 100.0)

property_color  (background, _("Background color"), "none")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     lens_distortion
#define GEGL_OP_C_SOURCE lens-distortion.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define SQR(x) ((x)*(x))

#define MIN3(x,y,z) (MIN (MIN ((x),(y)),(z)))

#define MAX3(x,y,z) (MAX (MAX ((x),(y)),(z)))

#define MAX_WH     1024
#define CHUNK_SIZE 512


typedef struct
{
  gdouble centre_x;
  gdouble centre_y;
  gdouble mult_sq;
  gdouble mult_qd;
  gdouble rescale;
  gdouble brighten;
  gdouble norm;
} LensValues;


static void
reorder (gdouble *low,
         gdouble *high)
{
  gdouble temp;

  if (*low < *high) return;

  temp = *low;
  *low = *high;
  *high = temp;
}

static LensValues
lens_setup_calc (GeglProperties *o,
                 GeglRectangle   boundary)
{
  LensValues lens;

  lens.norm     = 4.0 / (SQR (boundary.width) + SQR (boundary.height));
  lens.centre_x = boundary.width  * (100.0 + o->x_shift) / 200.0;
  lens.centre_y = boundary.height * (100.0 + o->y_shift) / 200.0;
  lens.mult_sq  = o->main / 200.0;
  lens.mult_qd  = o->edge / 200.0;
  lens.rescale  = pow (2.0, - o->zoom / 100.0);
  lens.brighten = - o->brighten / 10.0;

  return lens;
}

static void
lens_get_source_coord (gdouble     i,
                       gdouble     j,
                       gdouble    *x,
                       gdouble    *y,
                       gdouble    *mag,
                       LensValues *lens)
{
  gdouble radius_sq, off_x, off_y, radius_mult;

  off_x = i - lens->centre_x;
  off_y = j - lens->centre_y;

  radius_sq = SQR (off_x) + SQR (off_y);

  radius_sq *= lens->norm;

  radius_mult = radius_sq * lens->mult_sq + SQR (radius_sq) * lens->mult_qd;

  *mag = radius_mult;

  radius_mult = lens->rescale * (1.0 + radius_mult);

  *x = lens->centre_x + radius_mult * off_x;
  *y = lens->centre_y + radius_mult * off_y;
}

/* FIXME: not 100% bullet proof */
static GeglRectangle
get_required (GeglRectangle       *boundary,
              const GeglRectangle *roi,
              GeglOperation       *operation)
{

  GeglProperties *o;
  GeglRectangle   area;
  LensValues      lens;
  gdouble         x1, y1, x2, y2, x3, y3, x4, y4, mag;
  gint            x, y, width, height;

  o = GEGL_PROPERTIES (operation);

  lens = lens_setup_calc (o, *boundary);

  x = roi->x;
  y = roi->y;
  width = roi->width;
  height = roi->height;

  lens_get_source_coord (x,         y,          &x1, &y1, &mag, &lens);
  lens_get_source_coord (x + width, y,          &x2, &y2, &mag, &lens);
  lens_get_source_coord (x,         y + height, &x3, &y3, &mag, &lens);
  lens_get_source_coord (x + width, y + height, &x4, &y4, &mag, &lens);

  /* This is ugly, and happens
   * with a crazy set of parameters */
  reorder (&x1, &x2);
  reorder (&x3, &x4);

  reorder (&y1, &y3);
  reorder (&y2, &y4);

  if (lens.centre_y > y && lens.centre_y < y + height)
    {
      gdouble x5, y5, x6, y6;

      lens_get_source_coord (x,         lens.centre_y, &x5, &y5, &mag, &lens);
      lens_get_source_coord (x + width, lens.centre_y, &x6, &y6, &mag, &lens);

      reorder (&x5, &x6);

      area.x = floor (MIN3 (x1, x3, x5)) - 1;
      area.width = ceil (MAX3 (x2, x4, x6)) + 3 - area.x;
    }
  else
    {
      area.x = floor (MIN (x1, x3)) - 1;
      area.width = ceil (MAX (x2, x4)) + 3 - area.x;
    }

  if (lens.centre_x > x && lens.centre_x < x + width)
    {
      gdouble x5, y5, x6, y6;

      lens_get_source_coord (lens.centre_x, y,          &x5, &y5, &mag, &lens);
      lens_get_source_coord (lens.centre_x, y + height, &x6, &y6, &mag, &lens);

      reorder (&y5, &y6);

      area.y = floor (MIN3 (y1, y2, y5)) - 1;
      area.height = ceil (MAX3 (y3, y4, y6)) + 3 - area.y;
    }
  else
    {
      area.y = floor (MIN (y1, y2)) - 1;
      area.height = ceil (MAX (y3, y4)) + 3 - area.y;
    }

  return area;
}

static void
clamp_area (GeglRectangle *area,
            gdouble        center_x,
            gdouble        center_y)
{
  if (center_x <= area->x || area->width < 1)
    {
      area->width = CLAMP (area->width, 1, MAX_WH);
    }
  else
    {
      area->x += area->width - CLAMP (area->width, 1, MAX_WH);
      area->width = CLAMP (area->width, 1, MAX_WH);
    }

  if (center_y <= area->y || area->height < 1)
    {
      area->height = CLAMP (area->height, 1, MAX_WH);
    }
  else
    {
      area->y += area->height - CLAMP (area->height, 1, MAX_WH);
      area->height = CLAMP (area->height, 1, MAX_WH);
    }
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle *boundary;

  boundary = gegl_operation_source_get_bounding_box (operation, "input");

  if (strcmp (input_pad, "input") || !boundary)
    return *GEGL_RECTANGLE (0, 0, 0, 0);

  return get_required (boundary, roi, operation);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/*
 * Catmull-Rom cubic interpolation XXX: FIXME: use gegl resampler instead of
 *                                             reimplementing cubic sampler here
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

  for (c = 0; c < 4 * 4; ++c)
    {
      verts[c] = vm1 * src[c] + v * src[c + row_stride] +
        vp1 * src[c + row_stride * 2] + vp2 * src[ c + row_stride * 3];
    }

  for (c = 0; c < 4; ++c)
    {
      gfloat result;

      result = um1 * verts[c] + u * verts[c + 4] +
        up1 * verts[c + 4 * 2] + up2 * verts[c + 4 * 3];

      if (c != 3)
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
                   LensValues          *lens,
                   gint                 xx,
                   gint                 yy,
                   GeglBuffer          *input,
                   gfloat              *background,
                   gint                 level)
{
  gdouble sx, sy, mag;
  gdouble brighten;
  gfloat  pixel_buffer [16 * 4], temp[4];
  gdouble dx, dy;
  gint    x_int, y_int, x = 0, y = 0, offset = 0;

  temp[0] = temp[1] = temp[2] = temp[3] = 0.0;

  lens_get_source_coord ((gdouble) xx, (gdouble) yy, &sx, &sy, &mag, lens);

  /* pseudo gamma transformation, since the input is scRGB */
  brighten = pow (MAX (1.0 + mag * lens->brighten, 0.0), 2.4);

  x_int = floor (sx);
  dx = sx - x_int;

  y_int = floor (sy);
  dy = sy - y_int;

  for (y = y_int - 1; y <= y_int + 2; y++)
    {
      for (x = x_int - 1; x <= x_int + 2; x++)
        {
          gint b;

          if (x < boundary->x || x >= (boundary->x + boundary->width) ||
              y < boundary->y || y >= (boundary->y + boundary->height))
            {
              for (b = 0; b < 4; b++)
                pixel_buffer[offset++] = background[b];
            }
          else
            {

              if (x >= extended->x && x < (extended->x + extended->width) &&
                  y >= extended->y && y < (extended->y + extended->height))
                {
                  gint src_off;
                  src_off = (y - extended->y) * extended->width * 4 +
                    (x - extended->x) * 4;

                  for (b = 0; b < 4; b++)
                    temp[b] = src_buf[src_off++];
                }
              else
                {
                  gegl_buffer_sample_at_level (input, x, y, NULL, temp,
                                      babl_format ("RGBA float"),
                                      level,
                                      GEGL_SAMPLER_LINEAR,
                                      GEGL_ABYSS_CLAMP);
                }

              for (b = 0; b < 4; b++)
                pixel_buffer[offset++] = temp[b];
            }
        }
    }

  lens_cubic_interpolate (pixel_buffer, temp, dx, dy, brighten);

  offset = (yy - result->y) * result->width * 4 + (xx - result->x) * 4;

  for (x = 0; x < 4; x++)
    dst_buf[offset++] = temp[x];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  LensValues      lens;
  GeglRectangle   boundary;
  gint            i, j;
  gfloat         *src_buf, *dst_buf;
  gfloat          background[4];

  boundary = *gegl_operation_source_get_bounding_box (operation, "input");
  lens     =  lens_setup_calc (o, boundary);

  src_buf = g_new0 (gfloat, SQR (MAX_WH) * 4);
  dst_buf = g_new0 (gfloat, SQR (CHUNK_SIZE) * 4);

  gegl_color_get_pixel (o->background, babl_format ("RaGaBaA float"), background);

  for (j = 0; (j-1) * CHUNK_SIZE < result->height; j++)
    for (i = 0; (i-1) * CHUNK_SIZE < result->width; i++)
      {
        GeglRectangle chunked_result;
        GeglRectangle area;
        gint          x, y;

        chunked_result = *GEGL_RECTANGLE (result->x + i * CHUNK_SIZE,
                                          result->y + j * CHUNK_SIZE,
                                          CHUNK_SIZE, CHUNK_SIZE);

        gegl_rectangle_intersect (&chunked_result, &chunked_result, result);

        if (chunked_result.width < 1  || chunked_result.height < 1)
          continue;

        area = get_required (&boundary, &chunked_result, operation);

        clamp_area (&area, lens.centre_x, lens.centre_y);

        gegl_buffer_get (input, &area, 1.0, babl_format ("RaGaBaA float"), src_buf,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

        for (y = chunked_result.y; y < chunked_result.y + chunked_result.height; y++)
          for (x = chunked_result.x; x < chunked_result.x + chunked_result.width; x++)
            {
              lens_distort_func (src_buf, dst_buf, &area, &chunked_result, &boundary,
                                 &lens, x, y, input, background, level);
            }

        gegl_buffer_set (output, &chunked_result, 0, babl_format ("RaGaBaA float"),
                         dst_buf, GEGL_AUTO_ROWSTRIDE);
      }

  g_free (dst_buf);
  g_free (src_buf);

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
    "<node operation='gegl:lens-distortion'>"
    "  <params>"
    "    <param name='main'>100</param>"
    "    <param name='zoom'>20</param>"
    "    <param name='edge'>100</param>"
    "    <param name='x-shift'>20</param>"
    "    <param name='y-shift'>20</param>"
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
  operation_class->get_required_for_output = get_required_for_output;

  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",                  "gegl:lens-distortion",
    "title",                 _("Lens Distortion"),
    "categories",            "blur",
    "position-dependent",    "true",
    "license",               "GPL3+",
    "reference-composition", composition,
    "description", _("Corrects barrel or pincushion lens distortion."),
    NULL);
}

#endif
