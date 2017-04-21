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

#ifdef GEGL_PROPERTIES

property_double (mask_radius, _("Mask Radius"), 10.0)
    value_range (0.0, 50.0)

property_double (sharpness, _("Sharpness"), 0.5)
    value_range (0.0, 1.0)

property_double (black, _("Percent Black"), 0.2)
    value_range (0.0, 1.0)

property_double (white, _("Percent White"), 0.2)
    value_range (0.0, 1.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     photocopy
#define GEGL_OP_C_SOURCE photocopy.c

#include "gegl-op.h"
#include <math.h>

#define THRESHOLD 0.75

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

  gegl_node_link_many (image, blur2, write2, NULL);
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
compute_ramp (GeglBuffer          *dest1,
              GeglBuffer          *dest2,
              const GeglRectangle *roi,
              gdouble              pct_black,
              gdouble              pct_white,
              gboolean             under_threshold,
              gdouble             *threshold_black,
              gdouble             *threshold_white)
{
  GeglBufferIterator *iter;

  gint    hist1[2000];
  gint    hist2[2000];
  gdouble diff;
  gint    count;

  iter = gegl_buffer_iterator_new (dest1, roi, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ,
                                   GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, dest2, roi, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READ,
                            GEGL_ABYSS_NONE);

  memset (hist1, 0, sizeof (int) * 2000);
  memset (hist2, 0, sizeof (int) * 2000);
  count = 0;

  while (gegl_buffer_iterator_next (iter))
  {
    gint n_pixels = iter->length;
    gfloat *ptr1  = iter->data[0];
    gfloat *ptr2  = iter->data[1];

    while (n_pixels--)
      {
        diff = *ptr1 / *ptr2;
        ptr1++;
        ptr2++;

        if (under_threshold)
          {
            if (diff < THRESHOLD && diff >= 0.0f)
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
  }

  *threshold_black = calculate_threshold (hist1, pct_black, count, 0);
  *threshold_white = calculate_threshold (hist2, pct_white, count, 1);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("Y float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("Y float"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferIterator *iter;

  GeglBuffer *dest1;
  GeglBuffer *dest2;

  gdouble diff;
  gdouble ramp_down;
  gdouble ramp_up;
  gdouble mult;

  grey_blur_buffer (input, o->sharpness, o->mask_radius, &dest1, &dest2);

  compute_ramp (dest1, dest2, result,
                o->black, o->white, TRUE,
                &ramp_down, &ramp_up);

  iter = gegl_buffer_iterator_new (dest1, result, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ,
                                   GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, dest2, result, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READ,
                            GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, output, result, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_WRITE,
                            GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gint    n_pixels  = iter->length;
      gfloat *ptr1      = iter->data[0];
      gfloat *ptr2      = iter->data[1];
      gfloat *out_pixel = iter->data[2];

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
          out_pixel++;
        }
    }

  g_object_unref (dest1);
  g_object_unref (dest2);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",          "gegl:photocopy",
    "categories",    "artistic",
    "license",       "GPL3+",
    "title",         _("Photocopy"),
    "reference-hash", "cc015c712b0a9d9137fcea18065d65e7",
    "description", _("Simulate color distortion produced by a copy machine"),
    NULL);
}
#endif
