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
 * Copyright 2014 <pippin@gimp.org>
 *
 */

#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (pan, _("pan"), -180.0, 360, 0.0,
       _("pan of camera"))
gegl_chant_double (tilt, _("tilt"), -180.0, 180.0, 0.0,
       _("tilt of camera"))
gegl_chant_double (spin, _("spin"), -360.0, 360.0, 0.0,
       _("spin of camera"))
gegl_chant_double (zoom, _("zoom"), 0.01, 1000.0, 100.0, 
       _("zoom of camera"))

gegl_chant_enum (sampler_type, _("Sampler"), GeglSamplerType, gegl_sampler_type,
                 GEGL_SAMPLER_CUBIC, _("Image resampling method to use"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "gnomonic-projection.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RaGaBaA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  return result;
}

/* formulas from:
 * http://mathworld.wolfram.com/GnomonicProjection.html
 */

static void inline
calc_long_lat (float x, float  y,
               float tilt, float pan, float spin,
               float sin_tilt, float cos_tilt,
               float *in_long, float *in_lat)
{
  float p, c;
  float longtitude, latitude;
  float sin_c, cos_c;

  p = sqrtf (x*x+y*y);
  c = atan2f (p, 1);

  sin_c = sinf(c);
  cos_c = cosf(c);

  latitude = asinf (cos_c * sin_tilt + ( y * sin_c * cos_tilt) / p);
  longtitude = pan + atan2f (x * sin_c, p * cos_tilt * cos_c - y * sin_tilt * sin_c);

  if (longtitude < 0)
    longtitude += M_PI * 2;

  *in_long = (longtitude / (M_PI * 2));
  *in_lat = ((latitude + M_PI/2) / M_PI);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  float pan = o->pan / 360 * M_PI * 2;
  float spin = o->spin / 360 * M_PI * 2;
  float zoom = o->zoom / 100.0;
  float tilt = o->tilt / 360 * M_PI * 2;
  float width;
  float height;
  const Babl           *format_io;
  GeglSampler          *sampler;
  GeglBufferIterator   *it;
  float sin_tilt, cos_tilt;
  float cos_spin, sin_spin;
  gint                  index_in, index_out;
  float xoffset = 0.5;
  GeglRectangle in_rect = *gegl_operation_source_get_bounding_box (operation, "input");
  GeglMatrix2  scale_matrix;
  GeglMatrix2 *scale = NULL;

  format_io = babl_format ("RaGaBaA float");
  sampler = gegl_buffer_sampler_new (input, format_io, o->sampler_type);

  while (pan > M_PI)
    pan -= M_PI;

  sin_tilt = sinf (tilt);
  cos_tilt = cosf (tilt);
  sin_spin = sinf (spin);
  cos_spin = cosf (spin);

  width = in_rect.height;
  height = width;
  xoffset = ((in_rect.width - height)/height) / 2 + 0.5;

  if (o->sampler_type == GEGL_SAMPLER_NOHALO ||
      o->sampler_type == GEGL_SAMPLER_LOHALO)
    scale = &scale_matrix;

    {
      float   ud = ((1.0/width))/zoom;
      float   vd = ((1.0/height))/zoom;
      it = gegl_buffer_iterator_new (output, result, level, format_io, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
      index_out = 0;

      while (gegl_buffer_iterator_next (it))
        {
          gint    i;
          gint    n_pixels = it->length;
          gint    x = it->roi->x; /* initial x                   */
          gint    y = it->roi->y; /*           and y coordinates */

          float   u0 = ((x/width) - xoffset)/zoom;
          float   u, v;

          float *in = it->data[index_in];
          float *out = it->data[index_out];

          u = u0;
          v = ((y/height) - 0.5) / zoom; 

          if (scale)
            {
              for (i=0; i<n_pixels; i++)
                {
                  float cx, cy;
#define gegl_unmap(xx,yy,ud,vd) { \
                  float rx, ry;\
                  calc_long_lat (\
                     xx * cos_spin - yy * sin_spin,\
                     yy * cos_spin + xx * sin_spin,\
                     tilt, pan, spin,\
                     sin_tilt, cos_tilt,\
                     &rx, &ry);\
                  ud = rx;vd = ry;}
                  gegl_sampler_compute_scale (scale_matrix, u, v);
                  gegl_unmap(u,v, cx, cy);
#undef gegl_unmap

                  gegl_sampler_get (sampler, cx * in_rect.width, cy * in_rect.height,
                                    scale, out, GEGL_ABYSS_NONE);
                  in  += 4;
                  out += 4;

                  /* update x, y and u,v coordinates */
                  x++;
                  u+=ud;
                  if (x >= (it->roi->x + it->roi->width))
                    {
                      x = it->roi->x;
                      y++;
                      u = u0;
                      v += vd;
                    }
                }
              }
            else
              {
                for (i=0; i<n_pixels; i++)
                  {
                    float cx, cy;

                    calc_long_lat (
                        u * cos_spin - v * sin_spin,
                        v * cos_spin + u * sin_spin,
                        tilt, pan, spin,
                        sin_tilt, cos_tilt,
                        &cx, &cy);

                    gegl_sampler_get (sampler, cx * in_rect.width, cy * in_rect.height,
                                      scale, out, GEGL_ABYSS_NONE);
                    in  += 4;
                    out += 4;

                    /* update x, y and u,v coordinates */
                    x++;
                    u+=ud;
                    if (x >= (it->roi->x + it->roi->width))
                      {
                        x = it->roi->x;
                        u = u0;
                        y++;
                        v += vd;
                      }
                  }
              }
        }
    }
  g_object_unref (sampler);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationFilterClass *filter_class;
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class  = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:gnomonic-projection",
    "categories" , "misc",
    "description", _("Perform a gnomonic / equilinear projection of a equirectangular input image. (renders virtual camera images from a panorama)"),
    NULL);
}
#endif
