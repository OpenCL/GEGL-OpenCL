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

static void
grey_blur_buffer (GeglBuffer  *input,
                  gdouble      glow_radius,
                  GeglBuffer **dest)
{
  GeglNode *gegl, *image, *write, *blur;
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
                "buffer", dest, NULL);

  gegl_node_link_many (image, blur, write, NULL);
  gegl_node_process (write);

  g_object_unref (gegl);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("Y'CbCrA float"));
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
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

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
  GeglRectangle *whole_region;
  gfloat *dst_tmp_ptr, *input_ptr;

  const Babl *rgba            = babl_format ("RGBA float");
  const Babl *ycbcra          = babl_format ("Y'CbCrA float");

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof(gfloat));
  dst_tmp = g_slice_alloc (result->width * result->height * sizeof(gfloat));
  dst_convert = g_slice_alloc (result->width * result->height * 4 * sizeof(gfloat));
  dest_tmp = gegl_buffer_new (whole_region, babl_format ("Y' float"));

  gegl_buffer_get (input,
                   result,
                   1.0,
                   babl_format ("Y'CbCrA float"),
                   dst_buf,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;
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
    input_ptr   +=4;
  }

  gegl_buffer_set_extent (dest_tmp, whole_region);
  gegl_buffer_set (dest_tmp, whole_region, 0, babl_format ("Y' float"), dst_tmp, GEGL_AUTO_ROWSTRIDE);
  grey_blur_buffer (dest_tmp, o->glow_radius, &dest);

  sampler = gegl_buffer_sampler_new (dest,
                    babl_format ("Y' float"),
                    GEGL_SAMPLER_LINEAR);

  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;

  babl_process (babl_fish (ycbcra ,rgba), dst_buf, dst_convert, n_pixels);

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

  g_slice_free1 (result->width * result->height * 4 * sizeof(gfloat), dst_buf);
  g_slice_free1 (result->width * result->height * sizeof(gfloat), dst_tmp);
  g_slice_free1 (result->width * result->height * sizeof(gfloat), dst_convert);

  g_object_unref (sampler);
  g_object_unref (dest);
  g_object_unref (dest_tmp);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  gegl_buffer_set_extent (input, whole_region);
  return  TRUE;
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
