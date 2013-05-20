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
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (mask_radius, _("Mask radius"),
                   0.0, 50.0, 7.0,
                   _("Mask radius"))

gegl_chant_double (pct_black, _("Percent black"),
                   0.0, 1.0, 0.2,
                   _("Percent black"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "cartoon.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define THRESHOLD 1.0

typedef struct {
  gdouble      prev_mask_radius;
  gdouble      prev_pct_black;
  gdouble      prev_ramp;
} Ramps;

static void
grey_blur_buffer (GeglBuffer  *input,
                  gdouble      mask_radius,
                  GeglBuffer **dest1,
                  GeglBuffer **dest2)
{
  GeglNode *gegl, *image, *write1, *write2, *grey, *blur1, *blur2;
  gdouble radius, std_dev1, std_dev2;

  gegl = gegl_node_new ();
  image = gegl_node_new_child (gegl,
                "operation", "gegl:buffer-source",
                "buffer", input,
                NULL);
  grey = gegl_node_new_child (gegl,
                "operation", "gegl:grey",
                NULL);

  radius   = 1.0;
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

  write1 = gegl_node_new_child(gegl,
                "operation", "gegl:buffer-sink",
                "buffer", dest1, NULL);

  write2 = gegl_node_new_child(gegl,
                "operation", "gegl:buffer-sink",
                "buffer", dest2, NULL);

  gegl_node_link_many (image, grey, blur1, write1, NULL);
  gegl_node_process (write1);

  gegl_node_link_many (grey, blur2, write2, NULL);
  gegl_node_process (write2);

  g_object_unref (gegl);
}

static gdouble
compute_ramp (GeglBuffer    *input,
              GeglOperation *operation,
              gdouble        pct_black)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle *whole_region;
  gint    n_pixels;
  gint    hist[100];
  gdouble diff;
  gint    count;
  gfloat pixel1, pixel2;
  gint x;
  gint y;
  gint i;
  gint sum;
  GeglSampler *sampler1;
  GeglSampler *sampler2;
  GeglBuffer *dest1, *dest2;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  grey_blur_buffer (input, o->mask_radius, &dest1, &dest2);

  sampler1 = gegl_buffer_sampler_new (dest1,
                                      babl_format ("Y' float"),
                                      GEGL_SAMPLER_LINEAR);

  sampler2 = gegl_buffer_sampler_new (dest2,
                                      babl_format ("Y' float"),
                                      GEGL_SAMPLER_LINEAR);

  n_pixels = whole_region->width * whole_region->height;
  memset (hist, 0, sizeof (int) * 100);
  count = 0;
  x = whole_region->x;
  y = whole_region->y;

  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
                        x,
                        y,
                        NULL,
                        &pixel1,
                        GEGL_ABYSS_NONE);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2,
                        GEGL_ABYSS_NONE);

      if (pixel2 != 0)
      {
        diff = (gdouble) pixel1 / (gdouble) pixel2;

        if (diff < 1.0)
        {
          hist[(int) (diff * 100)] += 1;
          count += 1;
        }
      }

      x++;
      if (x >= whole_region->x + whole_region->width)
        {
          x = whole_region->x;
          y++;
        }
    }

  g_object_unref (sampler1);
  g_object_unref (sampler2);
  g_object_unref (dest1);
  g_object_unref (dest2);

  if (pct_black == 0.0 || count == 0)
    return 1.0;

  sum = 0;
  for (i = 0; i < 100; i++)
  {
    sum += hist[i];
    if (((gdouble) sum / (gdouble) count) > pct_black)
      return (1.0 - (gdouble) i / 100.0);
  }

  return 0.0;

}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_operation_set_format (operation, "input",
                             babl_format ("Y'CbCrA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("Y'CbCrA float"));

 if(o->chant_data)
    {
      Ramps* ramps = (Ramps*) o->chant_data;

      /* hack so that thresholds are only calculated once */
      if(ramps->prev_mask_radius != o->mask_radius ||
      ramps->prev_pct_black    != o->pct_black)
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

  GeglBuffer  *dest1;
  GeglBuffer  *dest2;
  GeglSampler *sampler1;
  GeglSampler *sampler2;

  Ramps* ramps;

  gint n_pixels;
  gfloat pixel1, pixel2;
  gfloat *out_pixel;
  gint x;
  gint y;


  gdouble diff;
  Ramps *get_ramps;
  gdouble ramp;
  gdouble mult = 0.0;

  gfloat *dst_buf;
  GeglRectangle *whole_region;

  static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

  dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof(gfloat));

  gegl_buffer_get (input, result, 1.0, babl_format ("Y'CbCrA float"), dst_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);


  g_static_mutex_lock (&mutex);
  if(o->chant_data == NULL)
    {
      whole_region = gegl_operation_source_get_bounding_box (operation, "input");
      gegl_buffer_set_extent (input, whole_region);
      o->chant_data = g_slice_new (Ramps);
      ramps = (Ramps*) o->chant_data;
      ramps->prev_ramp = compute_ramp (input,operation,o->pct_black);
      ramps->prev_mask_radius = o->mask_radius;
      ramps->prev_pct_black   = o->pct_black;
    }
  g_static_mutex_unlock (&mutex);

  gegl_buffer_set_extent (input, result);
  grey_blur_buffer (input, o->mask_radius, &dest1, &dest2);

  sampler1 = gegl_buffer_sampler_new (dest1,
                                      babl_format ("Y' float"),
                                      GEGL_SAMPLER_LINEAR);

  sampler2 = gegl_buffer_sampler_new (dest2,
                                      babl_format ("Y' float"),
                                      GEGL_SAMPLER_LINEAR);

  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;

  out_pixel = dst_buf;
  get_ramps = (Ramps*) o->chant_data;
  ramp = get_ramps->prev_ramp;

  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
                        x,
                        y,
                        NULL,
                        &pixel1,
                        GEGL_ABYSS_NONE);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2,
                        GEGL_ABYSS_NONE);

      if (pixel2 != 0)
      {
        diff = (gdouble) pixel1 / (gdouble) pixel2;
        if (diff < THRESHOLD)
        {
          if (GEGL_FLOAT_EQUAL(ramp,0.0))
            mult = 0.0;
          else
            mult = (ramp - MIN (ramp, (THRESHOLD - diff))) / ramp;
        }
        else
          mult = 1.0;
      }

      *out_pixel = CLAMP(pixel1 * mult, 0.0, 1.0);

      out_pixel += 4;

      x++;
      if (x >= result->x + result->width)
      {
        x = result->x;
        y++;
      }
    }


  gegl_buffer_set (output, result, 0, babl_format ("Y'CbCrA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * 4 * sizeof(gfloat), dst_buf);

  g_object_unref (sampler1);
  g_object_unref (sampler2);
  g_object_unref (dest1);
  g_object_unref (dest2);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  gegl_buffer_set_extent (input, whole_region);
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
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize   = finalize;
  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "categories",  "artistic",
    "name",        "gegl:cartoon",
    "description", _("Simulate a cartoon by enhancing edges"),
    NULL);
}

#endif
