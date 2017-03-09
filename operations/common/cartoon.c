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

#ifdef GEGL_PROPERTIES

property_double (mask_radius, _("Mask radius"), 7.0)
    value_range (0.0, 50.0)

property_double (pct_black, _("Percent black"), 0.2)
    value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     cartoon
#define GEGL_OP_C_SOURCE cartoon.c

#include "gegl-op.h"
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
compute_ramp (GeglBuffer  *dest1,
              GeglBuffer  *dest2,
              gdouble      pct_black)
{
  GeglBufferIterator *iter;
  gint    hist[100];
  gint    count;
  gint i;
  gint sum;

  memset (hist, 0, sizeof (int) * 100);
  count = 0;

  iter = gegl_buffer_iterator_new (dest1, NULL, 0, babl_format ("Y' float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest2, NULL, 0, babl_format ("Y' float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *pixel1   = iter->data[0];
      gfloat *pixel2   = iter->data[1];
      glong   n_pixels = iter->length;

      while (n_pixels--)
        {
          if (*pixel2 != 0)
            {
              gdouble diff = (gdouble) *pixel1 / (gdouble) *pixel2;

              if (diff < 1.0 && diff >= 0.0)
                {
                  hist[(int) (diff * 100)] += 1;
                  count += 1;
                }
            }

          pixel1++;
          pixel2++;
      }
  }

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
  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *region;

  region = gegl_operation_source_get_bounding_box (operation, "input");

  if (region != NULL)
    return *region;
  else
    return *GEGL_RECTANGLE (0, 0, 0, 0);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *output_roi)
{
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, input_pad);

  if (in_rect)
    return *in_rect;
  else
    return (GeglRectangle){0, 0, 0, 0};
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *output_roi)
{
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect)
    return *in_rect;
  else
    return (GeglRectangle){0, 0, 0, 0};
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o = GEGL_PROPERTIES (operation);
  GeglBufferIterator *iter;
  GeglBuffer         *dest1;
  GeglBuffer         *dest2;
  gdouble             ramp;
  gdouble             progress = 0.0;
  gdouble             pixels_count = result->width * result->height;

  grey_blur_buffer (input, o->mask_radius, &dest1, &dest2);

  ramp = compute_ramp (dest1, dest2, o->pct_black);

  iter = gegl_buffer_iterator_new (output, result, 0,
                                   babl_format ("Y'CbCrA float"),
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, input, result, 0,
                            babl_format ("Y'CbCrA float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest1, NULL, 0, babl_format ("Y' float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest2, NULL, 0, babl_format ("Y' float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);


  gegl_operation_progress (operation, 0.0, "");

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_pixel = iter->data[0];
      gfloat *in_pixel  = iter->data[1];
      gfloat *grey1     = iter->data[2];
      gfloat *grey2     = iter->data[3];
      glong   n_pixels  = iter->length;

      progress += n_pixels / pixels_count;

      while (n_pixels--)
        {
          gdouble mult = 0.0;

          if (*grey2 != 0)
            {
              gdouble diff = (gdouble) *grey1 / (gdouble) *grey2;

              if (diff < THRESHOLD)
                {
                  if (GEGL_FLOAT_EQUAL (ramp, 0.0))
                    mult = 0.0;
                  else
                    mult = (ramp - MIN (ramp, (THRESHOLD - diff))) / ramp;
                }
              else
                mult = 1.0;
            }

          out_pixel[0] = CLAMP (*grey1 * mult, 0.0, 1.0);
          out_pixel[1] = in_pixel[1];
          out_pixel[2] = in_pixel[2];
          out_pixel[3] = in_pixel[3];

          out_pixel += 4;
          in_pixel  += 4;
          grey1++;
          grey2++;
        }

        gegl_operation_progress (operation, progress, "");
    }

  gegl_operation_progress (operation, 1.0, "");

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
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->threaded                = FALSE;
  operation_class->get_required_for_output = get_required_for_output;

  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "categories",  "artistic",
    "name",        "gegl:cartoon",
    "title",       _("Cartoon"),
    "reference-hash", "baaba985ceebdcd37bbc73552ec7ffc1",
    "license",     "GPL3+",
    "description", _("Simulates a cartoon, its result is similar to a black"
                     " felt pen drawing subsequently shaded with color. This"
                     " is achieved by enhancing edges and darkening areas that"
                     " are already distinctly darker than their neighborhood"),
    NULL);
}

#endif
