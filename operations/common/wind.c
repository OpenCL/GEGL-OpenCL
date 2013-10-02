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
 * Copyright Nigel Wetten
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (threshold, _("Threshold"), 0.0, 100.0, 10.0,
                   _("Higher values restrict the effect to fewer "
                     "areas of the image"))

gegl_chant_int (strength, _("Strength"), 1, 1000, 40,
                _("Higher values increase the magnitude of the effect"))

gegl_chant_seed (seed, _("Seed"), _("Random seed"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "wind.c"

#include "gegl-chant.h"
#include <stdlib.h>

typedef struct
{
  gint x;
  gint y;
} pair;

static guint     tuple_hash  (gconstpointer v);
static gboolean  tuple_equal (gconstpointer v1,
                              gconstpointer v2);
static void      get_pixel   (gint    x,
                              gint    y,
                              gint    buf_width,
                              gfloat *src_begin,
                              gfloat *dst);

static guint
tuple_hash (gconstpointer v)
{
  const pair *data = v;
  return (g_int_hash (&data->x) ^ g_int_hash (&data->y));
}

static gboolean
tuple_equal (gconstpointer v1,
             gconstpointer v2)
{
  const pair *data1 = v1;
  const pair *data2 = v2;
  return (g_int_equal (&data1->x, &data2->x) &&
          g_int_equal (&data1->y, &data2->y));
}

static void
get_pixel (gint    x,
           gint    y,
           gint    buf_width,
           gfloat *src_begin,
           gfloat *dst)
{
  gint b;
  gfloat* src = src_begin + 4*(x + buf_width*y);
  for (b = 0; b < 4; b++)
    {
      dst[b] = src[b];
    }
}

static void
get_derivative (gfloat   *pixel1,
                gfloat   *pixel2,
                gfloat   *derivative)
{
  gint i;
  for (i = 0; i < 4; i++)
    derivative[i] = pixel1[i] - pixel2[i];
}

static gboolean
threshold_exceeded (gfloat  *pixel1,
                    gfloat  *pixel2,
                    gfloat   threshold)
{
  gfloat derivative[4];
  gint i;
  gfloat sum;

  get_derivative (pixel1, pixel2, derivative);

  sum = 0.0;
  for (i = 0; i < 4; i++)
    sum += derivative[i];
  return ((sum / 4.0) > (threshold/100.0));
}

static void
calculate_bleed (GHashTable    *h,
                 gfloat        *data,
                 gfloat         threshold,
                 gfloat         max_length,
                 GeglRectangle *rect,
                 gint           seed)
{
  gint x, y;
  GRand *gr = g_rand_new_with_seed (seed);
  for (y = 0; y < rect->height; y++)
    {
      for (x = 0; x < rect->width - 3; x++)
        {
          gfloat pixel1[4];
          gfloat pixel2[4];
          get_pixel (x, y, rect->width, data, pixel1);
          get_pixel (x + 3, y, rect->width, data, pixel2);
          if (threshold_exceeded (pixel1,
                                  pixel2,
                                  threshold))
            {
              pair *k = g_new (pair, 1);
              gint *v = g_new (gint, 1);
              gint bleed_length = 1 + (gint)(g_rand_double (gr) * max_length);

              k->x = x;
              k->y = y;

              *v = bleed_length;
              g_hash_table_insert (h, k, v);
            }
        }
    }
  g_rand_free (gr);
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  if (o->chant_data)
    {
      g_hash_table_destroy (o->chant_data);
      o->chant_data = NULL;
    }

  op_area->left   = o->strength;
  op_area->right  = o->strength;
  op_area->top    = o->strength;
  op_area->bottom = o->strength;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  gfloat *src_buf;
  gfloat *dst_buf;

  gint n_pixels = result->width * result->height;
  GeglRectangle src_rect;

  gfloat *current_pix;
  gfloat *target_pix;
  gfloat *dst_pix;

  gint x, y;
  gint total_src_pixels;
  gint total_dst_pixels;

  gint bleed_max;
  gint bleed_index;
  gfloat blend_coefficient;

  GHashTable *bleed_table;

  static GMutex mutex = { 0, };

  g_mutex_lock (&mutex);
  if (!o->chant_data)
    {
      GeglRectangle *whole_rect = gegl_operation_source_get_bounding_box (operation, "input");
      gfloat *data = (gfloat*) gegl_buffer_linear_open (input, NULL, NULL, babl_format ("RGBA float"));

      bleed_table = g_hash_table_new_full (tuple_hash, tuple_equal, g_free, g_free);
      calculate_bleed (bleed_table, data, o->threshold, (gfloat) o->strength, whole_rect, o->seed);
      o->chant_data = bleed_table;
      gegl_buffer_linear_close (input, data);
    }
  g_mutex_unlock (&mutex);

  bleed_table = (GHashTable*) o->chant_data;

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_src_pixels = src_rect.width * src_rect.height;
  total_dst_pixels = result->width * result->height;

  src_buf = g_slice_alloc (4 * total_src_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (4 * total_dst_pixels * sizeof (gfloat));

  gegl_buffer_get (input,
                   &src_rect,
                   1.0,
                   babl_format ("RGBA float"),
                   src_buf,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  current_pix = src_buf + 4*(o->strength + src_rect.width * o->strength);
  dst_pix = dst_buf;
  x = 0;
  y = 0;
  n_pixels = result->width * result->height;
  bleed_max = 0;
  bleed_index = 0;
  while (n_pixels--)
    {
      gint i;
      pair key = {x + result->x, y + result->y};
      gint *bleed = g_hash_table_lookup (bleed_table, &key);

      if (x == 0) {
        for (i = 0; i < o->strength; i++)
          {
            pair key = {result->x - i, y + result->y};
            gint *bleed = g_hash_table_lookup (bleed_table, &key);
            if (bleed) {
              bleed_max = *bleed;
              bleed_index = *bleed - i;
              break;
            }
          }
      }

      for (i = 0; i < 4; i++)
        dst_pix[i] = current_pix[i];

      if (bleed)
        {
          gfloat blend_color[4];
          gfloat blend_amount[4];
          gfloat *blend_pix;

          bleed_max = *bleed;
          bleed_index = *bleed;
          target_pix = current_pix;
          blend_pix = current_pix - 12;
          for (i = 0; i < 4; i++)
            {
              blend_amount[i] = target_pix[i] - blend_pix[i];
              blend_color[i] = blend_pix[i] + blend_amount[i];
              dst_pix[i] = (2.0 * blend_color[i] + dst_pix[i])/3.0;
            }
        }
      else if (bleed_index > 0)
        {
          gfloat blend_color[4];
          gfloat blend_amount[4];
          gfloat *blend_pix;
          bleed_index--;
          blend_coefficient = 1.0 - ((gfloat) bleed_index)/(gfloat) bleed_max;
          blend_pix = current_pix - 4 * (bleed_max - bleed_index) - 12;
          target_pix = current_pix;
          for (i = 0; i < 4; i++)
            {
              blend_amount[i] = target_pix[i] - blend_pix[i];
              blend_color[i] = blend_pix[i] + blend_amount[i] * blend_coefficient;
              dst_pix[i] = (2.0 * blend_color[i] + dst_pix[i])/3.0;
            }
        }

      x++;
      current_pix += 4;
      dst_pix += 4;
      if (x >= result->width)
        {
          bleed_max = 0;
          bleed_index = 0;
          x = 0;
          y++;
          current_pix += 8 * o->strength;
        }
    }
  gegl_buffer_set (output,
                   result,
                   1,
                   babl_format ("RGBA float"),
                   dst_buf,
                   GEGL_AUTO_ROWSTRIDE);

  g_slice_free1 (4 * total_src_pixels * sizeof (gfloat), src_buf);
  g_slice_free1 (4 * total_dst_pixels * sizeof (gfloat), dst_buf);

  return TRUE;
}


static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_hash_table_destroy (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize   = finalize;
  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
                                 "categories" , "distort",
                                 "name"       , "gegl:wind",
                                 "description", _("Wind-like bleed effect"),
                                 NULL);
}
#endif
