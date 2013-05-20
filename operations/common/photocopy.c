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
 * Copyright 1997 Spencer Kimball
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (mask_radius, _("Mask Radius"),
                   0.0, 50.0, 10.0,
                   _("Mask Radius"))

gegl_chant_double (sharpness, _("Sharpness"),
                   0.0, 1.0, 0.5,
                   _("Sharpness"))

gegl_chant_double (black, _("Percent Black"),
                   0.0, 1.0, 0.2,
                   _("Percent Black"))

gegl_chant_double (white, _("Percent White"),
                   0.0, 1.0, 0.2,
                   _("Percent White"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "photocopy.c"

#include "gegl-chant.h"
#include <math.h>

#define THRESHOLD 0.75

typedef struct {
  gdouble      black;
  gdouble      white;
  gdouble      prev_mask_radius;
  gdouble      prev_sharpness;
  gdouble      prev_black;
  gdouble      prev_white;
} Ramps;

static void
grey_blur_buffer (GeglBuffer  *input,
                  gdouble      sharpness,
                  gdouble      mask_radius,
                  GeglBuffer **dest1,
                  GeglBuffer **dest2)
{
  GeglNode *gegl, *image, *write1, *write2, *blur1, *blur2;
  gdouble radius, std_dev1, std_dev2;

  gegl = gegl_node_new ();
  image = gegl_node_new_child (gegl,
                               "operation", "gegl:buffer-source",
                               "buffer", input,
                               NULL);

  radius   = MAX (1.0, 10 * (1.0 - sharpness));
  radius   = fabs (radius) + 1.0;
  std_dev1 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  radius   = fabs (mask_radius) + 1.0;
  std_dev2 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  blur1 =  gegl_node_new_child (gegl,
                                "operation", "gegl:gaussian-blur",
                                "std_dev_x", std_dev1,
                                "std_dev_y", std_dev1,
                                NULL);
  blur2 =  gegl_node_new_child (gegl,
                                "operation", "gegl:gaussian-blur",
                                "std_dev_x", std_dev2,
                                "std_dev_y", std_dev2,
                                NULL);

  write1 = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer", dest1, NULL);

  write2 = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer", dest2, NULL);

  gegl_node_link_many (image, blur1, write1, NULL);
  gegl_node_process (write1);

  gegl_node_link_many (image,blur2, write2, NULL);
  gegl_node_process (write2);

  g_object_unref (gegl);
}

static gdouble
calculate_threshold (gint    *hist,
                     gdouble  pct,
                     gint     count,
                     gint     under_threshold)
{
  gint    sum;
  gint    i;

  if (pct == 0.0 || count == 0)
    return (under_threshold ? 1.0 : 0.0);

  sum = 0;
  for (i = 0; i < 2000; i++)
    {
      sum += hist[i];
      if (((gdouble) sum / (gdouble) count) > pct)
        {
          if (under_threshold)
            return (THRESHOLD - (gdouble) i / 1000.0);
          else
            return ((gdouble) i / 1000.0 - THRESHOLD);
        }
    }

  return (under_threshold ? 0.0 : 1.0);
}

static void
compute_ramp (GeglBuffer    *input,
              GeglOperation *operation,
              gdouble        pct_black,
              gdouble        pct_white,
              gint           under_threshold,
              gdouble       *threshold_black,
              gdouble       *threshold_white)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle *whole_region;
  gint    n_pixels;
  gint    hist1[2000];
  gint    hist2[2000];
  gdouble diff;
  gint    count;
  GeglBuffer *dest1, *dest2;

  gfloat *src1_buf;
  gfloat *src2_buf;
  gfloat *dst_buf;

  gint total_pixels;

  gfloat* ptr1;
  gfloat* ptr2;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  grey_blur_buffer (input, o->sharpness, o->mask_radius, &dest1, &dest2);

  total_pixels = whole_region->width * whole_region->height;
  src1_buf = g_slice_alloc (total_pixels * sizeof (gfloat));
  src2_buf = g_slice_alloc (total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (total_pixels * sizeof (gfloat));

  gegl_buffer_get (dest1, whole_region, 1.0, babl_format ("Y float"),
                   src1_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (dest2, whole_region, 1.0, babl_format ("Y float"),
                   src2_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  n_pixels = whole_region->width * whole_region->height;

  memset (hist1, 0, sizeof (int) * 2000);
  memset (hist2, 0, sizeof (int) * 2000);
  count = 0;
  ptr1 = src1_buf;
  ptr2 = src2_buf;
  while (n_pixels--)
    {
      diff = *ptr1 / *ptr2;
      ptr1++;
      ptr2++;
      if (under_threshold)
        {
          if (diff < THRESHOLD)
            {
              hist2[(int) (diff * 1000)] ++;
              count ++;
            }
        }
      else
        {
          if (diff >= THRESHOLD && diff < 2.0)
            {
              hist1[(int) (diff * 1000)] ++;
              count ++;
            }
        }

    }

  g_object_unref (dest1);
  g_object_unref (dest2);

  g_slice_free1 (total_pixels * sizeof (gfloat), src1_buf);
  g_slice_free1 (total_pixels * sizeof (gfloat), src2_buf);
  g_slice_free1 (total_pixels * sizeof (gfloat), dst_buf);

  *threshold_black = calculate_threshold (hist1, pct_black, count, 0);
  *threshold_white = calculate_threshold (hist2, pct_white, count, 1);
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_operation_set_format (operation, "input",
                             babl_format ("Y float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("Y float"));
  if (o->chant_data)
    {
      Ramps* ramps = (Ramps*) o->chant_data;

      /* hack so that thresholds are only calculated once */
      if (ramps->prev_mask_radius != o->mask_radius ||
          ramps->prev_sharpness   != o->sharpness   ||
          ramps->prev_black       != o->black       ||
          ramps->prev_white       != o->white)
        {
          g_slice_free (Ramps, o->chant_data);
          o->chant_data = NULL;
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBuffer *dest1;
  GeglBuffer *dest2;

  Ramps* ramps;

  gint n_pixels;
  gfloat *out_pixel;
  gint total_pixels;

  gdouble diff;
  Ramps *get_ramps;
  gdouble ramp_down;
  gdouble ramp_up;
  gdouble mult;

  gfloat *src1_buf;
  gfloat *src2_buf;
  gfloat *dst_buf;

  gfloat* ptr1;
  gfloat* ptr2;

  static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

  total_pixels = result->width * result->height;
  dst_buf = g_slice_alloc (total_pixels * sizeof (gfloat));

  g_static_mutex_lock (&mutex);
  if (o->chant_data == NULL)
    {
      o->chant_data = g_slice_new (Ramps);
      ramps = (Ramps*) o->chant_data;
      compute_ramp (input,
                    operation,
                    o->black,
                    o->white,1,
                    &ramps->black,
                    &ramps->white);
      ramps->prev_mask_radius = o->mask_radius;
      ramps->prev_sharpness   = o->sharpness;
      ramps->prev_black       = o->black;
      ramps->prev_white       = o->white;
    }
  g_static_mutex_unlock (&mutex);

  grey_blur_buffer (input, o->sharpness, o->mask_radius, &dest1, &dest2);

  total_pixels = result->width * result->height;
  src1_buf = g_slice_alloc (total_pixels * sizeof (gfloat));
  src2_buf = g_slice_alloc (total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (total_pixels * sizeof (gfloat));

  gegl_buffer_get (dest1, result, 1.0, babl_format ("Y float"),
                   src1_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (dest2, result, 1.0, babl_format ("Y float"),
                   src2_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  g_object_unref (dest1);
  g_object_unref (dest2);

  out_pixel = dst_buf;
  n_pixels = result->width * result->height;

  get_ramps = (Ramps*) o->chant_data;
  ramp_down = get_ramps->black;
  ramp_up = get_ramps->white;

  ptr1 = src1_buf;
  ptr2 = src2_buf;

  while (n_pixels--)
    {
      diff = *ptr1 / *ptr2;
      if (diff < THRESHOLD)
        {
          if (ramp_down == 0.0)
            *out_pixel = 0.0;
          else
            {
              mult = (ramp_down - MIN (ramp_down,
                                       (THRESHOLD - diff))) / ramp_down;
              *out_pixel = *ptr1 * mult;
            }
        }
      else
        {
          if (ramp_up == 0.0)
            mult = 1.0;
          else
            mult = MIN (ramp_up,
                        (diff - THRESHOLD)) / ramp_up;

          *out_pixel = mult + *ptr1 - mult * *ptr1;
        }
      ptr1++;
      ptr2++;
      out_pixel ++;
    }

  gegl_buffer_set (output,
                   result,
                   0,
                   babl_format ("Y float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_slice_free1 (total_pixels * sizeof (gfloat), src1_buf);
  g_slice_free1 (total_pixels * sizeof (gfloat), src2_buf);
  g_slice_free1 (total_pixels * sizeof (gfloat), dst_buf);

  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_slice_free (Ramps, o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass               *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:photocopy",
    "categories",  "artistic",
    "description", _("Simulate color distortion produced by a copy machine"),
    NULL);
}
#endif
