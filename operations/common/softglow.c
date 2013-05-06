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

gegl_chant_double (glow_radius, _("Glow radius"),   1.0, 50.0, 10.0, _("Glow radius"))

gegl_chant_double (brightness, _("Brightness"),   0.0, 1.0, 0.75, _("Brightness"))

gegl_chant_double (sharpness, _("Sharpness"),   0.0, 1.0, 0.85, _("Sharpness"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "softglow.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define SIGMOIDAL_BASE   2
#define SIGMOIDAL_RANGE  20

static GeglBuffer *
grey_blur_buffer (GeglBuffer  *input,
                  gdouble      glow_radius)
{
  GeglNode *gegl, *image, *write, *blur;
  GeglBuffer *dest;
  gdouble radius, std_dev;

  gegl = gegl_node_new ();
  image = gegl_node_new_child (gegl,
                "operation", "gegl:buffer-source",
                "buffer", input,
                NULL);

  radius   = fabs (glow_radius) + 1.0;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  blur =  gegl_node_new_child (gegl,
                "operation", "gegl:gaussian-blur",
                "std_dev_x", std_dev,
                "std_dev_y", std_dev,
                NULL);

  write = gegl_node_new_child (gegl,
                "operation", "gegl:buffer-sink",
                "buffer", &dest, NULL);

  gegl_node_link_many (image, blur, write, NULL);
  gegl_node_process (write);

  g_object_unref (gegl);

  return dest;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o    = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = ceil (fabs (o->glow_radius)) +1;
  area->top = area->bottom = ceil (fabs (o->glow_radius)) +1;

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
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o    = GEGL_CHANT_PROPERTIES (operation);

  GeglBuffer *dest, *dest_tmp;
  GeglSampler *sampler;

  gint n_pixels;
  gfloat pixel;
  gfloat *out_pixel;
  gint x;
  gint y;
  gint b;

  gfloat tmp;
  gdouble val;

  gfloat *dst_buf, *dst_tmp, *dst_convert;
  GeglRectangle  working_region;
  GeglRectangle *whole_region;
  gfloat *dst_tmp_ptr, *input_ptr;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  working_region.x      = result->x - area->left;
  working_region.width  = result->width + area->left + area->right;
  working_region.y      = result->y - area->top;
  working_region.height = result->height + area->top + area->bottom;

  gegl_rectangle_intersect (&working_region, &working_region, whole_region);

  dst_buf = g_slice_alloc (working_region.width * working_region.height * sizeof (gfloat));
  dst_tmp = g_slice_alloc (working_region.width * working_region.height * sizeof (gfloat));
  dest_tmp = gegl_buffer_new (&working_region, babl_format ("Y' float"));
  dst_convert = g_slice_alloc (result->width * result->height * 4 * sizeof (gfloat));


  gegl_buffer_get (input,
                   &working_region,
                   1.0,
                   babl_format ("Y' float"),
                   dst_buf,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  gegl_buffer_get (input,
                   result,
                   1.0,
                   babl_format ("RGBA float"),
                   dst_convert,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  n_pixels = working_region.width * working_region.height;
  dst_tmp_ptr = dst_tmp;
  input_ptr = dst_buf;

  while (n_pixels--)
  {
    /* compute sigmoidal transfer */
    val = *input_ptr;
    val = 1.0 / (1.0 + exp (-(SIGMOIDAL_BASE + (o->sharpness * SIGMOIDAL_RANGE)) * (val - 0.5)));
    val = val * o->brightness;
    *dst_tmp_ptr = CLAMP (val, 0.0, 1.0);

    dst_tmp_ptr +=1;
    input_ptr   +=1;
  }

  gegl_buffer_set (dest_tmp, &working_region, 0, babl_format ("Y' float"), dst_tmp, GEGL_AUTO_ROWSTRIDE);

  dest = grey_blur_buffer (dest_tmp, o->glow_radius);

  sampler = gegl_buffer_sampler_new (dest,
                                     babl_format ("Y' float"),
                                     GEGL_SAMPLER_LINEAR);

  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;

  out_pixel = dst_convert;

  while (n_pixels--)
    {
      gegl_sampler_get (sampler,
                        x,
                        y,
                        NULL,
                        &pixel,
                        GEGL_ABYSS_NONE);

      for (b = 0; b < 3; b++)
      {
        tmp = (1.0 - out_pixel[b]) * (1.0 - pixel) ;
        out_pixel[b] = CLAMP(1.0 - tmp, 0.0, 1.0);
      }

      out_pixel += 4;

      x++;
      if (x>=result->x + result->width)
        {
          x=result->x;
          y++;
        }
    }

  gegl_buffer_set (output,
                   result,
                   0,
                   babl_format ("RGBA float"),
                   dst_convert,
                   GEGL_AUTO_ROWSTRIDE);

  g_slice_free1 (working_region.width * working_region.height * sizeof (gfloat), dst_buf);
  g_slice_free1 (working_region.width * working_region.height * sizeof (gfloat), dst_tmp);
  g_slice_free1 (result->width * result->height * 4 * sizeof (gfloat), dst_convert);

  g_object_unref (sampler);
  g_object_unref (dest);
  g_object_unref (dest_tmp);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
                "categories" , "artistic",
                "name"       , "gegl:softglow",
                "description", _("Softglow effect"),
                NULL);
}

#endif
