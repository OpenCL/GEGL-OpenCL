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
 * 2002, 2014, 2016 (c) Øyvind Kolås pippin@gimp.org
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_color (from_0, _("From 0"), "black")
property_color (to_0, _("To 0"), "black")
property_double (weight_0, _("weight 0"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_1, _("From 1"), "black")
property_color (to_1, _("To 1"), "black")
property_double (weight_1, _("weight 1"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_2, _("From 2"), "black")
property_color (to_2, _("To 2"), "black")
property_double (weight_2, _("weight 2"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_3, _("From 3"), "black")
property_color (to_3, _("To 3"), "black")
property_double (weight_3, _("weight 3"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_4, _("From 4"), "black")
property_color (to_4, _("To 4"), "black")
property_double (weight_4, _("weight 4"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_5, _("From 5"), "black")
property_color (to_5, _("To 5"), "black")
property_double (weight_5, _("weight 5"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_6, _("From 6"), "black")
property_color (to_6, _("To 6"), "black")
property_double (weight_6, _("weight 6"), 100.0)
             ui_range (0.0, 220.0)
property_color (from_7, _("From 7"), "black")
property_color (to_7, _("To 7"), "black")
property_double (weight_7, _("weight 7"), 100.0)
             ui_range (0.0, 220.0)
property_double (weight, _("global weight scale"), 1.0)
             ui_range (0.0, 1.0)
property_double (amount, _("amount"), 1.0)
             ui_range (0.0, 1.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME color_warp
#define GEGL_OP_C_SOURCE color-warp.c

#include "gegl-op.h"
#include <math.h>

#define MAX_PAIRS 64

typedef struct CoordPair {
  float a[3];
  float b[3];
  float weight;
} CoordPair;

typedef struct CoordWarp {
  CoordPair pair[MAX_PAIRS];
  int count;
} CoordWarp;

static CoordWarp *cw_new (void)
{
  CoordWarp *cw;
  cw = g_new0 (CoordWarp, 1);
  return cw;
}

static void cw_clear_pairs (CoordWarp   *cw)
{
  cw->count = 0;
}

static void cw_destroy (CoordWarp   *cw)
{
  g_free (cw);
}

static void cw_add_pair (CoordWarp   *cw,
                         const float *coord_a,
                         const float *coord_b,
                         float        weight)
{
  int d;
  if (cw->count +1 >= MAX_PAIRS)
    return;
  for (d = 0; d < 3; d++)
    cw->pair[cw->count].a[d] = coord_a[d];
  for (d = 0; d < 3; d++)
    cw->pair[cw->count].b[d] = coord_b[d];
  cw->pair[cw->count].weight = weight;
  cw->count++;
}

static inline float sq_dist (CoordWarp *cw, const float *coord_a, const float *coord_b)
{
  int d;
  float sq_sum = 0;
  for (d = 0; d < 3; d++)
    sq_sum += (coord_b[d] - coord_a[d]) * (coord_b[d] - coord_a[d]);
  return sq_sum;
}

static inline float calc_weight (float dist, float lowest_dist, float coord_weight)
{
  float influence = coord_weight;
  return expf (-dist / influence);
}

static void cw_map (CoordWarp   *cw,
                    const float *coord_a,
                    float       *coord_b)
{
  int i;
  int lowest_dist_no = 0;
  double lowest_dist = 12345678900000.0;
  double sum_wc = 0.0;
  float warp[3] = {0,};

  for (i = 0; i < cw->count; i++)
  {
    float sqd = sq_dist (cw, coord_a, cw->pair[i].a);
    if (sqd < lowest_dist)
    {
      lowest_dist = sqd;
      lowest_dist_no = i;
    }
  }

  for (i = 0; i < cw->count; i++)
  {
    float sqd = sq_dist (cw, coord_a, cw->pair[i].a);
    sum_wc += lowest_dist / sqd;
  }

  if (lowest_dist > 0.0)
  {
    for (i = 0; i < cw->count; i++)
    {
      int d;
      float sqd = sq_dist (cw, coord_a, cw->pair[i].a);
      float weight = calc_weight (sqd, lowest_dist, cw->pair[i].weight);
      weight = weight / sum_wc;

      for (d = 0; d < 3; d++)
        warp[d] += (cw->pair[i].a[d] - cw->pair[i].b[d]) * weight;
    }
  }
  else
  {
    int d;
    for (d = 0; d < 3; d++)
      warp[d] = (cw->pair[lowest_dist_no].a[d] - cw->pair[lowest_dist_no].b[d]);
  }

  {
    int d;
    for (d = 0; d < 3; d++)
      coord_b[d] = coord_a[d] - warp[d];
  }
}

static void maybe_add_pair (CoordWarp *cw,
                            GeglColor *colorA,
                            GeglColor *colorB,
                            float      weight)
{
  gfloat from[4];
  gfloat to[4];
  const Babl *colorformat = babl_format("CIE Lab float");
  gegl_color_get_pixel (colorA, colorformat, from);
  gegl_color_get_pixel (colorB, colorformat, to);
  if (from[0] == 0.0 &&
      from[1] == 0.0 &&
      from[2] == 0.0 &&
      to[0] == 0.0 &&
      to[1] == 0.0 &&
      to[2] == 0.0)
  {
  }
  else
  {
    cw_add_pair (cw, from, to, weight);
  }
}

static void prepare (GeglOperation *operation)
{
  CoordWarp *cw;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = babl_format ("CIE Lab float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  if (o->user_data == NULL)
    o->user_data = cw_new ();
  cw = o->user_data;

  cw_clear_pairs (cw);
  maybe_add_pair (cw, o->from_0, o->to_0, o->weight * o->weight_0);
  maybe_add_pair (cw, o->from_1, o->to_1, o->weight * o->weight_1);
  maybe_add_pair (cw, o->from_2, o->to_2, o->weight * o->weight_2);
  maybe_add_pair (cw, o->from_3, o->to_3, o->weight * o->weight_3);
  maybe_add_pair (cw, o->from_4, o->to_4, o->weight * o->weight_4);
  maybe_add_pair (cw, o->from_5, o->to_5, o->weight * o->weight_5);
  maybe_add_pair (cw, o->from_6, o->to_6, o->weight * o->weight_6);
  maybe_add_pair (cw, o->from_7, o->to_7, o->weight * o->weight_7);
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      cw_destroy (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o         = GEGL_PROPERTIES (op);
  gfloat         *in_pixel  = in_buf;
  gfloat         *out_pixel = out_buf;
  CoordWarp      *cw        = o->user_data;
  float           amount    = o->amount;

  in_pixel  = in_buf;
  out_pixel = out_buf;

  while (n_pixels--)
    {
      if (amount == 1.0)
      {
        cw_map (cw, in_pixel, out_pixel);
      }
      else
      {
        int d;
        float res[4];
        cw_map (cw, in_pixel, res);
        for (d = 0; d < 3; d++)
          out_pixel[d] = in_pixel[d] * (1.0-amount) + res[d] * amount;
      }
      in_pixel  += 3;
      out_pixel += 3;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  object_class       = G_OBJECT_CLASS (klass);
  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  object_class->finalize      = finalize;
  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-warp",
    "title",       _("Color warp"),
    "categories",  "color",
    "description", _("Warps the colors of an image between colors with weighted distortion factors, color pairs which are black to black get ignored when constructing the mapping."),
    NULL);
}

#endif
