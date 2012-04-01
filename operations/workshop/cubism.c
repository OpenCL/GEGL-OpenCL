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
 * Copyright 2011 Robert Sasu <sasu.robert@gmail.com>
 *
 * Based on "Cubism" GIMP plugin
 * Copyright (C) 1996 Spencer Kimball, Tracy Scott
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 * Speedups by Elliot Lee
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (tile_size, _("Tile size"), 0.0, 256.0, 10.0,
                   _("Tile size"))
gegl_chant_double (tile_saturation, _("Tile saturation"), 0.0, 10.0, 2.5,
                   _("Tile saturation"))
gegl_chant_int (seed, _("Seed"), 0 , G_MAXINT, 1,
                _("Random seed"))


# else

#define GEGL_CHANT_TYPE_AREA_FILTER

#define GEGL_CHANT_C_FILE       "cubism.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

#define SCALE_WIDTH     125
#define BLACK             0
#define BG                1
#define SUPERSAMPLE       4
#define MAX_POINTS        4
#define RANDOMNESS        5

typedef struct
{
  gint x,y;
} Vector2;

typedef struct
{
  gint        npts;
  Vector2     pts[MAX_POINTS];
} Polygon;


static void prepare (GeglOperation *operation)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  gint tmp;
  /*
   * Calculate the needed extension for the ROI
   *      MAX (o->tile_size +
   *           g_rand_double_range (gr, 0, o->tile_size / 4.0) -
   *           o->tile_size / 8.0) * o->tile_saturation)
   */

  tmp = ceil ((9 * o->tile_size / 8.0) * o->tile_saturation);

  op_area->left = op_area->right = op_area->top = op_area->bottom = tmp;

  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static inline gdouble
calc_alpha_blend (gdouble *vec,
                  gdouble  one_over_dist,
                  gdouble  x,
                  gdouble  y)
{
  gdouble r;

  if (! one_over_dist)
    return 1.0;

  r = (vec[0] * x + vec[1] * y) * one_over_dist;

  return CLAMP (r, 0.2, 1.0);
}


static void
convert_segment (gint  x1,
                 gint  y1,
                 gint  x2,
                 gint  y2,
                 gint  offset,
                 gint *min,
                 gint *max)
{
  gint ydiff, y, tmp;
  gdouble xinc, xstart;


  if (y1 > y2)
    {
      tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp;
    }
  ydiff = (y2 - y1);

  if (ydiff)
    {
      xinc = (gdouble) (x2 - x1) / (gdouble) ydiff;
      xstart = x1 + 0.5 * xinc;
      for (y = y1 ; y < y2; y++)
        {
          if (xstart < min[y - offset])
            min[y-offset] = xstart;
          if (xstart > max[y - offset])
            max[y-offset] = xstart;

          xstart += xinc;
        }
    }
}

static void
randomize_indices (gint   count,
                   gint  *indices,
                   GRand *gr)
{
  gint i;
  gint index1, index2;
  gint tmp;

  for (i = 0; i < count * RANDOMNESS; i++)
    {
      index1 = g_rand_int_range (gr, 0, count);
      index2 = g_rand_int_range (gr, 0, count);
      tmp = indices[index1];
      indices[index1] = indices[index2];
      indices[index2] = tmp;
    }

}

static void
polygon_add_point (Polygon *poly,
                   gdouble  x,
                   gdouble  y)
{
  if (poly->npts < MAX_POINTS)
    {
      poly->pts[poly->npts].x = x;
      poly->pts[poly->npts].y = y;
      poly->npts++;
    }
  else
    g_print ("Unable to add additional point.\n");
}

static void
polygon_rotate (Polygon *poly,
                gdouble  theta)
{
  gint i;
  gdouble ct, st;
  gdouble ox, oy;

  ct = cos (theta);
  st = sin (theta);

  for (i = 0; i < poly->npts; i++)
    {
      ox = poly->pts[i].x;
      oy = poly->pts[i].y;
      poly->pts[i].x = ct * ox - st * oy;
      poly->pts[i].y = st * ox + ct * oy;
    }
}

static gint
polygon_extents (Polygon *poly,
                 gdouble *x1,
                 gdouble *y1,
                 gdouble *x2,
                 gdouble *y2)
{
  gint i;

  if (!poly->npts)
    return 0;

  *x1 = *x2 = poly->pts[0].x;
  *y1 = *y2 = poly->pts[0].y;

  for (i = 1; i < poly->npts; i++)
    {
      if (poly->pts[i].x < *x1)
        *x1 = poly->pts[i].x;
      if (poly->pts[i].x > *x2)
        *x2 = poly->pts[i].x;
      if (poly->pts[i].y < *y1)
        *y1 = poly->pts[i].y;
      if (poly->pts[i].y > *y2)
        *y2 = poly->pts[i].y;
    }

  return 1;
}

static void
polygon_translate (Polygon *poly,
                   gdouble  tx,
                   gdouble  ty)
{
  gint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x += tx;
      poly->pts[i].y += ty;
    }
}

static void
polygon_reset (Polygon *poly)
{
  poly->npts = 0;
}

static void
fill_poly_color (Polygon             *poly,
                 const GeglRectangle *extended,
                 const GeglRectangle *boundary,
                 gfloat              *dst_buf,
                 gfloat              *color)
{
  gdouble       dmin_x = 0.0;
  gdouble       dmin_y = 0.0;
  gdouble       dmax_x = 0.0;
  gdouble       dmax_y = 0.0;
  gint          xs, ys;
  gint          xe, ye;
  gint          min_x, min_y;
  gint          max_x, max_y;
  gint          size_x, size_y;
  gint         *max_scanlines, *max_scanlines_iter;
  gint         *min_scanlines, *min_scanlines_iter;
  gdouble       val;
  gfloat        alpha;
  gfloat        buf[4];
  gint          i, j, x, y;
  gdouble       sx, sy;
  gdouble       ex, ey;
  gdouble       xx, yy;
  gdouble       vec[2];
  gdouble       dist, one_over_dist;
  gint          x1, y1, x2, y2;
  gint         *vals, *vals_iter, *vals_end;
  gint          b;

  sx = poly->pts[0].x;
  sy = poly->pts[0].y;
  ex = poly->pts[1].x;
  ey = poly->pts[1].y;

  dist = sqrt (pow((ex - sx),2) + pow((ey - sy),2));

  if (dist > 0.0)
    {
      one_over_dist = 1.0 / dist;
      vec[0] = (ex - sx) * one_over_dist;
      vec[1] = (ey - sy) * one_over_dist;
    }
  else
    {
      one_over_dist = 0.0;
      vec[0] = 0.0;
      vec[1] = 0.0;
    }

  x1 = boundary->x;
  y1 = boundary->y;
  x2 = boundary->x + boundary->width;
  y2 = boundary->y + boundary->height;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * SUPERSAMPLE;
  size_x = (max_x - min_x) * SUPERSAMPLE;

  min_scanlines = min_scanlines_iter = g_new0 (gint, size_y);
  max_scanlines = max_scanlines_iter = g_new0 (gint, size_y);

  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * SUPERSAMPLE;
      max_scanlines[i] = min_x * SUPERSAMPLE;
    }

  if (poly->npts)
    {
      Vector2 *curptr;
      gint     poly_npts = poly->npts;

      xs = (gint) (poly->pts[poly_npts-1].x);
      ys = (gint) (poly->pts[poly_npts-1].y);
      xe = (gint) poly->pts[0].x;
      ye = (gint) poly->pts[0].y;

      xs *= SUPERSAMPLE;
      ys *= SUPERSAMPLE;
      xe *= SUPERSAMPLE;
      ye *= SUPERSAMPLE;

      convert_segment (xs, ys, xe, ye, min_y * SUPERSAMPLE,
                       min_scanlines, max_scanlines);

      for (i = 1, curptr = &poly->pts[0]; i < poly_npts; i++)
        {
          xs = (gint) curptr->x;
          ys = (gint) curptr->y;
          curptr++;
          xe = (gint) curptr->x;
          ye = (gint) curptr->y;

          xs *= SUPERSAMPLE;
          ys *= SUPERSAMPLE;
          xe *= SUPERSAMPLE;
          ye *= SUPERSAMPLE;

          convert_segment (xs, ys, xe, ye, min_y * SUPERSAMPLE,
                           min_scanlines, max_scanlines);
        }
    }

  vals = g_new0 (gint, size_x);

  for (i = 0; i < size_y; i++, min_scanlines_iter++, max_scanlines_iter++)
    {
      if (! (i % SUPERSAMPLE))
        {
          memset (vals, 0, sizeof (gint) * size_x);
        }

      yy = (gdouble)i / (gdouble)SUPERSAMPLE + min_y;

      for (j = *min_scanlines_iter; j < *max_scanlines_iter; j++)
        {
          x = j - min_x * SUPERSAMPLE;
          vals[x] += 1;
        }

      if (! ((i + 1) % SUPERSAMPLE))
        {
          y = (i / SUPERSAMPLE) + min_y;

          if (y >= y1 && y < y2)
            {
              for (j = 0; j < size_x; j += SUPERSAMPLE)
                {
                  x = (j / SUPERSAMPLE) + min_x;

                  if (x >= x1 && x < x2)
                    {
                      for (val = 0, vals_iter = &vals[j],
                             vals_end = &vals_iter[SUPERSAMPLE];
                           vals_iter < vals_end;
                           vals_iter++)
                        val += *vals_iter;

                      val /= pow((SUPERSAMPLE),2);

                      if (val > 0)
                        {
                          xx = (gdouble) j / (gdouble) SUPERSAMPLE + min_x;
                          alpha = (gfloat) (val * calc_alpha_blend (vec,
                                                                    one_over_dist,
                                                                    xx - sx,
                                                                    yy - sy));

                          for (b = 0; b < 4; b++)
                            buf[b] = dst_buf[( (y-extended->y) * extended->width
                                               + (x-extended->x)) * 4 + b];

                          for (b = 0; b < 4; b++)
                            buf[b] = (color[b] * alpha) + (buf[b] * (1 - alpha));

                          for (b = 0; b < 4; b++)
                            dst_buf[((y-extended->y) * extended->width +
                                     (x - extended->x)) * 4 + b] = buf[b];

                        }
                    }
                }
            }
        }
    }

  g_free (vals);
  g_free (min_scanlines);
  g_free (max_scanlines);
}


static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy(&result, in_rect);

  return result;
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o            = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area      = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            boundary     = get_effective_area (operation);
  GeglRectangle            extended;
  const Babl              *format       = babl_format ("RGBA float");

  GRand  *gr = g_rand_new_with_seed (o->seed);
  gfloat  color[4];
  gint    cols, rows, num_tiles, count;
  gint   *random_indices;
  gfloat *dst_buf;

  Polygon poly;
  gint    i;

  extended.x      = CLAMP (result->x - op_area->left, boundary.x, boundary.x +
                           boundary.width);
  extended.width  = CLAMP (result->width + op_area->left + op_area->right, 0,
                           boundary.width);
  extended.y      = CLAMP (result->y - op_area->top, boundary.y, boundary.y +
                           boundary.width);
  extended.height = CLAMP (result->height + op_area->top + op_area->bottom, 0,
                           boundary.height);

  dst_buf = g_new0 (gfloat, extended.width * extended.height * 4);

  cols = (result->width + o->tile_size - 1) / o->tile_size;
  rows = (result->height + o->tile_size - 1) / o->tile_size;

  num_tiles = (rows + 1) * (cols + 1);

  random_indices = g_new0 (gint, num_tiles);

  for (i = 0; i < num_tiles; i++)
    random_indices[i] = i;

  randomize_indices (num_tiles, random_indices, gr);

  for (count = 0; count < num_tiles; count++)
    {
      gint i, j, ix, iy;
      gdouble x, y, width, height, theta;

      i = random_indices[count] / (cols + 1);
      j = random_indices[count] % (cols + 1);

      x = j * o->tile_size + (o->tile_size / 4.0)
        - g_rand_double_range (gr, 0, (o->tile_size /2.0)) + result->x;

      y = i * o->tile_size + (o->tile_size / 4.0)
        - g_rand_double_range (gr, 0, (o->tile_size /2.0)) + result->y;

      width  = (o->tile_size +
                g_rand_double_range (gr, -o->tile_size / 8.0, o->tile_size / 8.0))
        * o->tile_saturation;

      height = (o->tile_size +
                g_rand_double_range (gr, -o->tile_size / 8.0, o->tile_size / 8.0))
        * o->tile_saturation;

      theta = g_rand_double_range (gr, 0, 2 * G_PI);

      polygon_reset (&poly);
      polygon_add_point (&poly, -width / 2.0, -height / 2.0);
      polygon_add_point (&poly,  width / 2.0,  -height / 2.0);
      polygon_add_point (&poly,  width / 2.0,   height / 2.0);
      polygon_add_point (&poly, -width / 2.0,  height / 2.0);
      polygon_rotate (&poly, theta);
      polygon_translate (&poly, x, y);

      ix = CLAMP (x, boundary.x, boundary.x + boundary.width - 1);
      iy = CLAMP (y, boundary.y, boundary.y + boundary.height - 1);

      gegl_buffer_sample (input, ix, iy, NULL, color, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      fill_poly_color (&poly, &extended, &boundary, dst_buf, color);
    }

  gegl_buffer_set (output, &extended, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);
  g_free (random_indices);
  g_free (gr);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (!in_rect){
    return result;
  }

  gegl_rectangle_copy(&result, in_rect);

#ifdef TRACE
  g_warning ("< get_bounding_box result = %dx%d+%d+%d", result.width, result.height, result.x, result.y);
#endif
  return result;
}


/* Compute the input rectangle required to compute the specified region of interest (roi).
 */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle  result = get_effective_area (operation);

#ifdef TRACE
  g_warning ("> get_required_for_output src=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
  if (roi)
    g_warning ("  ROI == %dx%d+%d+%d", roi->width, roi->height, roi->x, roi->y);
#endif

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "categories", "artistic",
    "name", "gegl:cubism",
    "description",  _("A filter that somehow resembles a cubist painting style"),
    NULL);
}

#endif
