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

#ifdef GEGL_PROPERTIES

property_double (threshold, _("Threshold"), 10.0)
    description(_("Higher values restrict the effect to fewer areas of the image"))
    value_range (0, 100)

property_int (strength, _("Strength"), 40)
    description(_("Higher values increase the magnitude of the effect"))
    value_range(1,1000)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE wind.c

#include "gegl-op.h"
#include <stdlib.h>

typedef struct
{
  gint x;
  gint y;
} pair;

static guint     tuple_hash  (gconstpointer v);
static gboolean  tuple_equal (gconstpointer v1,
                              gconstpointer v2);
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
calculate_bleed (GeglOperation *operation,
                 GeglBuffer    *input)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle rectA, rectB;
  GeglBufferIterator *iter;
  gfloat max_length = (gfloat) o->strength;
  gfloat threshold  = o->threshold;
  GHashTable *bleed_table = o->user_data;

  rectA = *gegl_operation_source_get_bounding_box (operation, "input");
  rectA.width -= 3;
  rectB = rectA;
  rectB.x += 3;

  if (rectA.width <= 0)
    return;

  iter = gegl_buffer_iterator_new (input,
                                   &rectA,
                                   0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READ,
                                   GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter,
                            input,
                            &rectB,
                            0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_READ,
                            GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gint ix, iy;
      gfloat *pixelsA = (gfloat *)iter->data[0];
      gfloat *pixelsB = (gfloat *)iter->data[1];

      for (ix = 0; ix < iter->roi[0].width; ix++)
        for (iy = 0; iy < iter->roi[0].height; iy++)
          {
            gint idx = iy * iter->roi[0].width + ix * 4;
            if (threshold_exceeded (&pixelsA[idx], &pixelsB[idx], threshold))
              {
                gint x = ix + iter->roi[0].x;
                gint y = iy + iter->roi[0].y;
                pair *k = g_new (pair, 1);
                gint *v = g_new (gint, 1);
                gint bleed_length = 1 + gegl_random_int_range (o->rand, x, y, 0, 0, 0, max_length);

                k->x = x;
                k->y = y;

                *v = bleed_length;
                g_hash_table_insert (bleed_table, k, v);
              }
          }
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties          *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_PROPERTIES (operation);

  if (o->user_data)
    {
      g_hash_table_destroy (o->user_data);
      o->user_data = NULL;
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
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
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
  if (!o->user_data)
    {
      o->user_data = g_hash_table_new_full (tuple_hash, tuple_equal, g_free, g_free);
      calculate_bleed (operation, input);
    }
  g_mutex_unlock (&mutex);

  bleed_table = (GHashTable*) o->user_data;

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_src_pixels = src_rect.width * src_rect.height;
  total_dst_pixels = result->width * result->height;

  src_buf = gegl_malloc (4 * total_src_pixels * sizeof (gfloat));
  dst_buf = gegl_malloc (4 * total_dst_pixels * sizeof (gfloat));

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

  gegl_free (src_buf);
  gegl_free (dst_buf);

  return TRUE;
}


static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      g_hash_table_destroy (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
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
                                 "categories", "distort",
                                 "name",       "gegl:wind",
                                 "title",      _("Wind"),
                                 "license",    "GPL3+",
                                 "description", _("Wind-like bleed effect"),
                                 NULL);
}
#endif
