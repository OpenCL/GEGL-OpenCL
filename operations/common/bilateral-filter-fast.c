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
 * Copyright 2012 Victor Oliveira <victormatheus@gmail.com>
 */

 /* This is an implementation of a fast approximated bilateral filter
  * algorithm descripted in:
  *
  *  A Fast Approximation of the Bilateral Filter using a Signal Processing Approach
  *  Sylvain Paris and Frédo Durand
  *  European Conference on Computer Vision (ECCV'06)
  */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (r_sigma, _("Smoothness"),
                   1, 100, 50,
                   _("Level of smoothness"))

gegl_chant_int (s_sigma, _("Blur radius"),
                1, 100, 8,
                _("Radius of square pixel region, (width and height will be radius*2+1)."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "bilateral-filter-fast.c"

#include "gegl-chant.h"
#include <math.h>

inline static float lerp(float a, float b, float v)
{
  return (1.0f - v) * a + v * b;
}

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gint                 s_sigma,
                  gfloat               r_sigma);

static gboolean
bilateral_cl_process (GeglOperation       *operation,
                      GeglBuffer          *input,
                      GeglBuffer          *output,
                      const GeglRectangle *result,
                      gint                 s_sigma,
                      gfloat               r_sigma);

#include <stdio.h>

static void bilateral_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
bilateral_get_required_for_output (GeglOperation       *operation,
                                  const gchar         *input_pad,
                                  const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}


static GeglRectangle
bilateral_get_cached_region (GeglOperation       *operation,
                            const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static gboolean
bilateral_process (GeglOperation       *operation,
                   GeglBuffer          *input,
                   GeglBuffer          *output,
                   const GeglRectangle *result,
                   gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);

  if (gegl_cl_is_accelerated () && bilateral_cl_process (operation, input, output, result, o->s_sigma, o->r_sigma/100))
      return TRUE;

  bilateral_filter (input, result, output, result, o->s_sigma, o->r_sigma/100);

  return  TRUE;
}

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gint                 s_sigma,
                  gfloat               r_sigma)
{
  gint c, x, y, z, k, i;

  const gint padding_xy = 2;
  const gint padding_z  = 2;

  const gint width    = src_rect->width;
  const gint height   = src_rect->height;
  const gint channels = 4;

  const gint sw = (width -1) / s_sigma + 1 + (2 * padding_xy);
  const gint sh = (height-1) / s_sigma + 1 + (2 * padding_xy);
  const gint depth = (int)(1.0f / r_sigma) + 1 + (2 * padding_z);

  /* down-sampling */

  gfloat *grid     = g_new0 (gfloat, sw * sh * depth * channels * 2);
  gfloat *blurx    = g_new0 (gfloat, sw * sh * depth * channels * 2);
  gfloat *blury    = g_new0 (gfloat, sw * sh * depth * channels * 2);
  gfloat *blurz    = g_new0 (gfloat, sw * sh * depth * channels * 2);

  gfloat *input    = g_new0 (gfloat, width * height * channels);
  gfloat *smoothed = g_new0 (gfloat, width * height * channels);

  #define INPUT(x,y,c) input[c+channels*(x + width * y)]

  #define  GRID(x,y,z,c,i) grid [i+2*(c+channels*(x+sw*(y+z*sh)))]
  #define BLURX(x,y,z,c,i) blurx[i+2*(c+channels*(x+sw*(y+z*sh)))]
  #define BLURY(x,y,z,c,i) blury[i+2*(c+channels*(x+sw*(y+z*sh)))]
  #define BLURZ(x,y,z,c,i) blurz[i+2*(c+channels*(x+sw*(y+z*sh)))]

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), input, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  for (k=0; k < (sw * sh * depth * channels * 2); k++)
    {
      grid [k] = 0.0f;
      blurx[k] = 0.0f;
      blury[k] = 0.0f;
      blurz[k] = 0.0f;
    }

#if 0
  /* in case we want to normalize the color space */

  gfloat input_min[4] = { FLT_MAX,  FLT_MAX,  FLT_MAX};
  gfloat input_max[4] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  for(y = 0; y < height; y++)
    for(x = 0; x < width; x++)
      for(c = 0; c < channels; c++)
        {
          input_min[c] = MIN(input_min[c], INPUT(x,y,c));
          input_max[c] = MAX(input_max[c], INPUT(x,y,c));
        }
#endif

  /* downsampling */

  for(y = 0; y < height; y++)
    for(x = 0; x < width; x++)
      for(c = 0; c < channels; c++)
        {
          const float z = INPUT(x,y,c); // - input_min[c];

          const int small_x = (int)((float)(x) / s_sigma + 0.5f) + padding_xy;
          const int small_y = (int)((float)(y) / s_sigma + 0.5f) + padding_xy;
          const int small_z = (int)((float)(z) / r_sigma + 0.5f) + padding_z;

          g_assert(small_x >= 0 && small_x < sw);
          g_assert(small_y >= 0 && small_y < sh);
          g_assert(small_z >= 0 && small_z < depth);

          GRID(small_x, small_y, small_z, c, 0) += INPUT(x, y, c);
          GRID(small_x, small_y, small_z, c, 1) += 1.0f;
        }

  /* blur in x, y and z */
  /* XXX: we could use less memory, but at expense of code readability */

  for (z = 1; z < depth-1; z++)
    for (y = 1; y < sh-1; y++)
      for (x = 1; x < sw-1; x++)
        for(c = 0; c < channels; c++)
          for (i=0; i<2; i++)
            BLURX(x, y, z, c, i) = (GRID (x-1, y, z, c, i) + 2.0f * GRID (x, y, z, c, i) + GRID (x+1, y, z, c, i)) / 4.0f;

  for (z = 1; z < depth-1; z++)
    for (y = 1; y < sh-1; y++)
      for (x = 1; x < sw-1; x++)
        for(c = 0; c < channels; c++)
          for (i=0; i<2; i++)
            BLURY(x, y, z, c, i) = (BLURX (x, y-1, z, c, i) + 2.0f * BLURX (x, y, z, c, i) + BLURX (x, y+1, z, c, i)) / 4.0f;

  for (z = 1; z < depth-1; z++)
    for (y = 1; y < sh-1; y++)
      for (x = 1; x < sw-1; x++)
        for(c = 0; c < channels; c++)
          for (i=0; i<2; i++)
            BLURZ(x, y, z, c, i) = (BLURY (x, y-1, z, c, i) + 2.0f * BLURY (x, y, z, c, i) + BLURY (x, y+1, z, c, i)) / 4.0f;

  /* trilinear filtering */

  for (y=0; y < height; y++)
    for (x=0; x < width; x++)
      for(c = 0; c < channels; c++)
        {
          float xf = (float)(x) / s_sigma + padding_xy;
          float yf = (float)(y) / s_sigma + padding_xy;
          float zf = INPUT(x,y,c) / r_sigma + padding_z;

          int x1 = CLAMP((int)xf, 0, sw-1);
          int y1 = CLAMP((int)yf, 0, sh-1);
          int z1 = CLAMP((int)zf, 0, depth-1);

          int x2 = CLAMP(x1+1, 0, sw-1);
          int y2 = CLAMP(y1+1, 0, sh-1);
          int z2 = CLAMP(z1+1, 0, depth-1);

          float x_alpha = xf - x1;
          float y_alpha = yf - y1;
          float z_alpha = zf - z1;

          float interpolated[2];

          g_assert(xf >= 0 && xf < sw);
          g_assert(yf >= 0 && yf < sh);
          g_assert(zf >= 0 && zf < depth);

          for (i=0; i<2; i++)
              interpolated[i] =
              lerp(lerp(lerp(BLURZ(x1, y1, z1, c, i), BLURZ(x2, y1, z1, c, i), x_alpha),
                        lerp(BLURZ(x1, y2, z1, c, i), BLURZ(x2, y2, z1, c, i), x_alpha), y_alpha),
                   lerp(lerp(BLURZ(x1, y1, z2, c, i), BLURZ(x2, y1, z2, c, i), x_alpha),
                        lerp(BLURZ(x1, y2, z2, c, i), BLURZ(x2, y2, z2, c, i), x_alpha), y_alpha), z_alpha);

          smoothed[channels*(y*width+x)+c] = interpolated[0] / interpolated[1];
        }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), smoothed,
                   GEGL_AUTO_ROWSTRIDE);

  g_free (grid);
  g_free (blurx);
  g_free (blury);
  g_free (blurz);
  g_free (input);
  g_free (smoothed);
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"
#include "opencl/bilateral-filter-fast.cl.h"

GEGL_CL_STATIC;

static gboolean
cl_bilateral (cl_mem                in_tex,
              cl_mem                out_tex,
              const GeglRectangle  *roi,
              const GeglRectangle  *src_rect,
              gint                  s_sigma,
              gfloat                r_sigma)
{
  cl_int cl_err = 0;

  gint c;

  const gint width    = src_rect->width;
  const gint height   = src_rect->height;

  const gint sw = (width -1) / s_sigma + 1;
  const gint sh = (height-1) / s_sigma + 1;
  const gint depth = (int)(1.0f / r_sigma) + 1;

  size_t global_ws[2];
  size_t local_ws[2];

  cl_mem grid = NULL;
  cl_mem blur[4] = {NULL, NULL, NULL, NULL};

  GEGL_CL_BUILD(bilateral_filter_fast,
                "bilateral_downsample",
                "bilateral_blur",
                "bilateral_interpolate")

  grid = gegl_clCreateBuffer (gegl_cl_get_context (),
                              CL_MEM_READ_WRITE,
                              sw * sh * depth * sizeof(cl_float8),
                              NULL,
                              &cl_err);
  CL_CHECK;

  for(c = 0; c < 4; c++)
    {
      blur[c] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                     CL_MEM_READ_WRITE,
                                     sw * sh * depth * sizeof(cl_float2),
                                     NULL, &cl_err);
      CL_CHECK;
    }

  {
  local_ws[0] = 8;
  local_ws[1] = 8;

  global_ws[0] = ((sw + local_ws[0] - 1)/local_ws[0])*local_ws[0];
  global_ws[1] = ((sh + local_ws[1] - 1)/local_ws[1])*local_ws[1];

  GEGL_CL_ARG_START(cl_data->kernel[0])
  GEGL_CL_ARG(cl_mem,   in_tex)
  GEGL_CL_ARG(cl_mem,   grid)
  GEGL_CL_ARG(cl_int,   width)
  GEGL_CL_ARG(cl_int,   height)
  GEGL_CL_ARG(cl_int,   sw)
  GEGL_CL_ARG(cl_int,   sh)
  GEGL_CL_ARG(cl_int,   depth)
  GEGL_CL_ARG(cl_int,   s_sigma)
  GEGL_CL_ARG(cl_float, r_sigma)
  GEGL_CL_ARG_END

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, global_ws, local_ws,
                                       0, NULL, NULL);
  CL_CHECK;
  }

  {
  local_ws[0] = 16;
  local_ws[1] = 16;

  global_ws[0] = ((sw + local_ws[0] - 1)/local_ws[0])*local_ws[0];
  global_ws[1] = ((sh + local_ws[1] - 1)/local_ws[1])*local_ws[1];

  GEGL_CL_ARG_START(cl_data->kernel[1])
  GEGL_CL_ARG(cl_mem, grid)
  GEGL_CL_ARG(cl_mem, blur[0])
  GEGL_CL_ARG(cl_mem, blur[1])
  GEGL_CL_ARG(cl_mem, blur[2])
  GEGL_CL_ARG(cl_mem, blur[3])
  GEGL_CL_ARG(cl_int, sw)
  GEGL_CL_ARG(cl_int, sh)
  GEGL_CL_ARG(cl_int, depth)
  GEGL_CL_ARG_END

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[1], 2,
                                       NULL, global_ws, local_ws,
                                       0, NULL, NULL);
  CL_CHECK;
  }

  {
  global_ws[0] = width;
  global_ws[1] = height;

  GEGL_CL_ARG_START(cl_data->kernel[2])
  GEGL_CL_ARG(cl_mem,   in_tex)
  GEGL_CL_ARG(cl_mem,   blur[0])
  GEGL_CL_ARG(cl_mem,   blur[1])
  GEGL_CL_ARG(cl_mem,   blur[2])
  GEGL_CL_ARG(cl_mem,   blur[3])
  GEGL_CL_ARG(cl_mem,   out_tex)
  GEGL_CL_ARG(cl_int,   width)
  GEGL_CL_ARG(cl_int,   sw)
  GEGL_CL_ARG(cl_int,   sh)
  GEGL_CL_ARG(cl_int,   depth)
  GEGL_CL_ARG(cl_int,   s_sigma)
  GEGL_CL_ARG(cl_float, r_sigma)
  GEGL_CL_ARG_END

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[2], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  CL_CHECK;
  }

  cl_err = gegl_clFinish(gegl_cl_get_command_queue ());
  CL_CHECK;

  GEGL_CL_RELEASE(grid);

  for(c = 0; c < 4; c++)
    {
      GEGL_CL_RELEASE(blur[c]);
    }

  return FALSE;

error:
  if (grid) GEGL_CL_RELEASE(grid);

  for(c = 0; c < 4; c++)
    {
      if (blur[c]) GEGL_CL_RELEASE(blur[c]);
    }

  return TRUE;
}

static gboolean
bilateral_cl_process (GeglOperation       *operation,
                      GeglBuffer          *input,
                      GeglBuffer          *output,
                      const GeglRectangle *result,
                      gint                 s_sigma,
                      gfloat               r_sigma)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;
  gint j;
  cl_int cl_err;

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_CL_BUFFER_WRITE);
  gint read = gegl_buffer_cl_iterator_add (i, input, result, in_format, GEGL_CL_BUFFER_READ, GEGL_ABYSS_NONE);

  GEGL_CL_BUFFER_ITERATE_START(i, j, err)
    {
       err = cl_bilateral(i->tex[read][j],
                          i->tex[0][j],
                          &i->roi[0][j],
                          &i->roi[read][j],
                          s_sigma,
                          r_sigma);
    }
  GEGL_CL_BUFFER_ITERATE_END(err)

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = bilateral_process;

  operation_class->prepare                 = bilateral_prepare;
  operation_class->get_required_for_output = bilateral_get_required_for_output;
  operation_class->get_cached_region       = bilateral_get_cached_region;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
  "name"       , "gegl:bilateral-filter-fast",
  "categories" , "tonemapping",
  "description",
           _("A fast approximate implementation of the bilateral filter"),
        NULL);
}


#endif
