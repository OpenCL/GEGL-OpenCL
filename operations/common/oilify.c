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
 * Copyright 1995 Spencer Kimball and Peter Mattis
 * Copyright 1996 Torsten Martinsen
 * Copyright 2007 Daniel Richard G.
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (mask_radius, _("Mask Radius"), 1, 25, 4,
                _("Radius of circle around pixel"))

gegl_chant_int (exponent, _("Exponent"), 1, 20, 8,
                   _("Exponent"))

gegl_chant_int (intensities, _("Number of intensities"), 8, 256, 128,
                    _("Histogram size"))

gegl_chant_boolean (use_inten, _("Intensity Mode"), TRUE,
                    _("Use pixel luminance values"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "oilify.c"

#include "gegl-chant.h"
#include <math.h>

#define NUM_INTENSITIES       256

/* Get the pixel from x, y offset from the center pixel src_pix */
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
get_pixel_inten (gint    x,
                 gint    y,
                 gint    buf_width,
                 gfloat *inten_begin,
                 gfloat *dst)
{
  *dst = *(inten_begin + (x + buf_width*y));
}
static void
oilify_pixel_inten (gint     x,
                    gint     y,
                    gdouble  radius,
                    gint     exponent,
                    gint     intensities,
                    gint     buf_width,
                    gfloat  *src_buf,
                    gfloat  *inten_buf,
                    gfloat  *dst_pixel)
{
  gfloat cumulative_rgb[4][NUM_INTENSITIES];
  gint   hist_inten[NUM_INTENSITIES];
  gfloat mult_inten;
  gfloat temp_pixel[4];
  gint   ceil_radius = ceil (radius);
  gdouble radius_sq = radius*radius;
  gint i, j, b;
  gint inten_max;
  gint intensity;
  gfloat ratio, temp_inten_pixel;
  gfloat weight;
  gfloat color[4];
  gfloat div;
  for (i = 0; i < intensities; i++)
    {
      hist_inten[i] = 0;
      for (b = 0; b < 4; b++)
          cumulative_rgb[b][i] = 0.0;
    }

  /* calculate histograms */
  for (i = -ceil_radius; i <= ceil_radius; i++)
    {
      for (j = -ceil_radius; j <= ceil_radius; j++)
        {
          if (i*i + j*j <= radius_sq)
            {
              get_pixel (x + i,
                         y + j,
                         buf_width,
                         src_buf,
                         temp_pixel);
              get_pixel_inten (x + i,
                               y + j,
                               buf_width,
                               inten_buf,
                               &temp_inten_pixel);
              intensity = temp_inten_pixel * (intensities - 1);
              hist_inten[intensity]++;
              for (b = 0; b < 4; b++)
                {
                  cumulative_rgb[b][intensity] += temp_pixel[b];
                }
            }
        }
    }

  inten_max = 1;

  /* calculated maximums */
  for (i = 0; i < intensities; i++) {
    inten_max = MAX (inten_max, hist_inten[i]);
  }

  /* calculate weight and use it to set the pixel */
  div = 0.0;

  for (b = 0; b < 4; b++)
    color[b] = 0.0;
  for (i = 0; i < intensities; i++)
    {
      if (hist_inten[i] > 0)
      {
        ratio = (gfloat) hist_inten[i] / (gfloat) inten_max;

        /* using this instead of pow function gives HUGE performance improvement
           but we cannot use floating point exponent... */
        weight = 1.;
        for(j = 0; j < exponent; j++)
          weight *= ratio;
        /* weight = powf(ratio, exponent); */
        mult_inten = weight / (gfloat) hist_inten[i];

        div += weight;
        for (b = 0; b < 4; b++)
          color[b] += mult_inten * cumulative_rgb[b][i];
      }
    }
  for (b = 0; b < 4; b++)
    dst_pixel[b] = color[b]/div;
}

static void
oilify_pixel (gint           x,
              gint           y,
              gdouble        radius,
              gint           exponent,
              gint           intensities,
              gint           buf_width,
              gfloat        *src_buf,
              gfloat        *dst_pixel)
{
  gint   hist[4][NUM_INTENSITIES];
  gfloat temp_pixel[4];
  gint   ceil_radius = ceil (radius);
  gdouble radius_sq = radius*radius;
  gint i, j, b;
  gint hist_max[4];
  gint intensity;
  gfloat sum[4];
  gfloat ratio;
  gfloat weight;
  gfloat result[4];
  gfloat div[4];
  for (i = 0; i < intensities; i++)
    {
      for (b = 0; b < 4; b++)
        {
          hist[b][i] = 0;
        }
    }

  /* calculate histograms */
  for (i = -ceil_radius; i <= ceil_radius; i++)
    {
      for (j = -ceil_radius; j <= ceil_radius; j++)
        {
          if (i*i + j*j <= radius_sq)
            {
              get_pixel (x + i,
                         y + j,
                         buf_width,
                         src_buf,
                         temp_pixel);
              for (b = 0; b < 4; b++)
                {
                  intensity = temp_pixel[b] * (intensities - 1);
                  hist[b][intensity]++;
                }
            }
        }
    }
    for (b = 0; b < 4; b++)
      hist_max[b] = 1;
    for (i = 0; i < intensities; i++) {
      for (b = 0; b < 4; b++)
        if(hist_max[b] < hist[b][i]) /* MAX macros too slow here */
          hist_max[b] = hist[b][i];
    }

  /* calculate weight and use it to set the pixel */

    for (b = 0; b < 4; b++)
      {
        sum[b] = 0.0;
        div[b] = 0.0;
      }
    for (i = 0; i < intensities; i++)
      {
        /* UNROLL this bottleneck loop, up to 50% faster */
        #define DO_HIST_STEP(b) if(hist[b][i] > 0)                          \
          {                                                                 \
            ratio = (gfloat) hist[b][i] / (gfloat) hist_max[b];             \
            weight = 1.;                                                    \
            for(j = 0; j < exponent; j++)                                   \
              weight *= ratio;                                              \
            sum[b] += weight * (gfloat) i;                                  \
            div[b] += weight;                                               \
          }

        DO_HIST_STEP(0)
        DO_HIST_STEP(1)
        DO_HIST_STEP(2)
        DO_HIST_STEP(3)
        #undef DO_HIST_STEP
      }

    for (b = 0; b < 4; b++)
      {
        result[b] = sum[b] / (gfloat) (intensities - 1);
        dst_pixel[b] = result[b]/div[b];
      }
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->mask_radius;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

/* two small different kernels are better than one big */
static const char* kernel_source =
"#define NUM_INTENSITIES 256                                                \n"
"kernel void kernel_oilify(global float4 *in,                               \n"
"                             global float4 *out,                           \n"
"                             const int mask_radius,                        \n"
"                             const int intensities,                        \n"
"                             const float exponent)                         \n"
"{                                                                          \n"
"  int gidx = get_global_id(0);                                             \n"
"  int gidy = get_global_id(1);                                             \n"
"  int x = gidx + mask_radius;                                              \n"
"  int y = gidy + mask_radius;                                              \n"
"  int dst_width = get_global_size(0);                                      \n"
"  int src_width = dst_width + mask_radius * 2;                             \n"
"  float4 hist[NUM_INTENSITIES];                                            \n"
"  float4 hist_max = 1.0;                                                   \n"
"  int i, j, intensity;                                                     \n"
"  int radius_sq = mask_radius * mask_radius;                               \n"
"  float4 temp_pixel;                                                       \n"
"  for (i = 0; i < intensities; i++)                                        \n"
"    hist[i] = 0.0;                                                         \n"
"                                                                           \n"
"  for (i = -mask_radius; i <= mask_radius; i++)                            \n"
"  {                                                                        \n"
"    for (j = -mask_radius; j <= mask_radius; j++)                          \n"
"      {                                                                    \n"
"        if (i*i + j*j <= radius_sq)                                        \n"
"          {                                                                \n"
"            temp_pixel = in[x + i + (y + j) * src_width];                  \n"
"            hist[(int)(temp_pixel.x * (intensities - 1))].x+=1;            \n"
"            hist[(int)(temp_pixel.y * (intensities - 1))].y+=1;            \n"
"            hist[(int)(temp_pixel.z * (intensities - 1))].z+=1;            \n"
"            hist[(int)(temp_pixel.w * (intensities - 1))].w+=1;            \n"
"          }                                                                \n"
"      }                                                                    \n"
"  }                                                                        \n"
"                                                                           \n"
"  for (i = 0; i < intensities; i++) {                                      \n"
"    if(hist_max.x < hist[i].x)                                             \n"
"      hist_max.x = hist[i].x;                                              \n"
"    if(hist_max.y < hist[i].y)                                             \n"
"      hist_max.y = hist[i].y;                                              \n"
"    if(hist_max.z < hist[i].z)                                             \n"
"      hist_max.z = hist[i].z;                                              \n"
"    if(hist_max.w < hist[i].w)                                             \n"
"      hist_max.w = hist[i].w;                                              \n"
"  }                                                                        \n"
"  float4 div = 0.0;                                                        \n"
"  float4 sum = 0.0;                                                        \n"
"  float4 ratio, weight;                                                    \n"
"  for (i = 0; i < intensities; i++)                                        \n"
"  {                                                                        \n"
"    ratio = hist[i] / hist_max;                                            \n"
"    weight = pow(ratio, (float4)exponent);                                 \n"
"    sum += weight * (float4)i;                                             \n"
"    div += weight;                                                         \n"
"  }                                                                        \n"
"  out[gidx + gidy * dst_width] = sum / div / (float)(intensities - 1);     \n"
"}                                                                          \n"
"                                                                           \n"
"kernel void kernel_oilify_inten(global float4 *in,                         \n"
"                             global float4 *out,                           \n"
"                             const int mask_radius,                        \n"
"                             const int intensities,                        \n"
"                             const float exponent)                         \n"
"{                                                                          \n"
"  int gidx = get_global_id(0);                                             \n"
"  int gidy = get_global_id(1);                                             \n"
"  int x = gidx + mask_radius;                                              \n"
"  int y = gidy + mask_radius;                                              \n"
"  int dst_width = get_global_size(0);                                      \n"
"  int src_width = dst_width + mask_radius * 2;                             \n"
"  float4 cumulative_rgb[NUM_INTENSITIES];                                  \n"
"  int hist_inten[NUM_INTENSITIES], inten_max;                              \n"
"  int i, j, intensity;                                                     \n"
"  int radius_sq = mask_radius * mask_radius;                               \n"
"  float4 temp_pixel;                                                       \n"
"  for (i = 0; i < intensities; i++)                                        \n"
"  {                                                                        \n"
"    hist_inten[i] = 0;                                                     \n"
"    cumulative_rgb[i] = 0.0;                                               \n"
"  }                                                                        \n"
"  for (i = -mask_radius; i <= mask_radius; i++)                            \n"
"  {                                                                        \n"
"    for (j = -mask_radius; j <= mask_radius; j++)                          \n"
"      {                                                                    \n"
"        if (i*i + j*j <= radius_sq)                                        \n"
"          {                                                                \n"
"            temp_pixel = in[x + i + (y + j) * src_width];                  \n"
"            /*Calculate intensity on the fly, GPU does it fast*/           \n"
"            intensity = (int)((0.299 * temp_pixel.x                        \n"
"                      +0.587 * temp_pixel.y                                \n"
"                      +0.114 * temp_pixel.z) * (float)(intensities-1));    \n"
"            hist_inten[intensity] += 1;                                    \n"
"            cumulative_rgb[intensity] += temp_pixel;                       \n"
"          }                                                                \n"
"      }                                                                    \n"
"  }                                                                        \n"
"  inten_max = 1;                                                           \n"
"                                                                           \n"
"  /* calculated maximums */                                                \n"
"  for (i = 0; i < intensities; i++) {                                      \n"
"    if(hist_inten[i] > inten_max)                                          \n"
"      inten_max = hist_inten[i];                                           \n"
"  }                                                                        \n"
"  float div = 0.0;                                                         \n"
"  float ratio, weight, mult_inten;                                         \n"
"                                                                           \n"
"  float4 color = 0.0;                                                      \n"
"  for (i = 0; i < intensities; i++)                                        \n"
"  {                                                                        \n"
"    if (hist_inten[i] > 0)                                                 \n"
"    {                                                                      \n"
"      ratio = (float)(hist_inten[i]) / (float)(inten_max);                 \n"
"      weight = pow(ratio, exponent);                                       \n"
"      mult_inten = weight / (float)(hist_inten[i]);                        \n"
"                                                                           \n"
"      div += weight;                                                       \n"
"      color += mult_inten * cumulative_rgb[i];                             \n"
"    }                                                                      \n"
"  }                                                                        \n"
"  out[gidx + gidy * dst_width] = color/div;                                \n"
"}                                                                          \n";


static GeglClRunData *cl_data = NULL;

static cl_int
cl_oilify (cl_mem              in_tex,
           cl_mem              out_tex,
           size_t              global_worksize,
           const GeglRectangle *roi,
           gint                mask_radius,
           gint                number_of_intensities,
           gint                exponent,
           gboolean            use_inten)
{
  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_oilify", "kernel_oilify_inten", NULL};
      cl_data = gegl_cl_compile_and_build(kernel_source, kernel_name);
    }
  if (!cl_data)  return 0;

  const size_t gbl_size[2] = {roi->width,roi->height};
  cl_int radius      = mask_radius;
  cl_int intensities = number_of_intensities;
  cl_float exp       = (gfloat)exponent;
  cl_int cl_err      = 0;
  gint arg = 0;

  /* simple hack: select suitable kernel using boolean, 0 - no intensity mode, 1 - intensity mode */
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[use_inten], arg++, sizeof(cl_mem), (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[use_inten], arg++, sizeof(cl_mem), (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[use_inten], arg++, sizeof(cl_int), (void*)&radius);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[use_inten], arg++, sizeof(cl_int), (void*)&intensities);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[use_inten], arg++, sizeof(cl_float), (void*)&exp);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(),
                                       cl_data->kernel[use_inten], 2,
                                       NULL, gbl_size, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  return CL_SUCCESS;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;
  gint j;
  cl_int cl_err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,result, out_format, GEGL_CL_BUFFER_WRITE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format, GEGL_CL_BUFFER_READ,
                                                           o->mask_radius, o->mask_radius, o->mask_radius, o->mask_radius, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_oilify(i->tex[read][j],
                         i->tex[0][j],
                         i->size[0][j],&i->roi[0][j],
                         o->mask_radius,
                         o->intensities,
                         o->exponent,
                         o->use_inten);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:oilify: %s", gegl_cl_errstring(cl_err));
        return FALSE;
      }
    }
  }
  return TRUE;
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

  gint x = o->mask_radius; /* initial x                   */
  gint y = o->mask_radius; /*           and y coordinates */
  gfloat *src_buf;
  gfloat *dst_buf;
  gfloat *inten_buf;
  gfloat *out_pixel;
  gint n_pixels = result->width * result->height;
  GeglRectangle src_rect;
  gint total_pixels;

  if (gegl_cl_is_accelerated ())
    if(cl_process(operation, input, output, result))
      return TRUE;

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_pixels = src_rect.width * src_rect.height;

  src_buf = g_slice_alloc (4 * total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (4 * n_pixels * sizeof (gfloat));
  if(o->use_inten)
    inten_buf = g_slice_alloc (total_pixels * sizeof (gfloat));

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  if(o->use_inten)
    gegl_buffer_get (input, &src_rect, 1.0, babl_format ("Y float"),
                   inten_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  out_pixel = dst_buf;

  while (n_pixels--)
    {
      if(o->use_inten)
        oilify_pixel_inten (x, y, o->mask_radius, o->exponent, o->intensities,
                   src_rect.width, src_buf, inten_buf, out_pixel);
      else
        oilify_pixel (x, y, o->mask_radius, o->exponent, o->intensities,
                   src_rect.width, src_buf, out_pixel);
      out_pixel += 4;

      /* update x and y coordinates */
      x++;
      if (x >= result->width + o->mask_radius)
        {
          x=o->mask_radius;
          y++;
        }
    }


  gegl_buffer_set (output, result, 0,
                   babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (4 * total_pixels * sizeof (gfloat), src_buf);
  g_slice_free1 (4 * n_pixels * sizeof (gfloat), dst_buf);
  if(o->use_inten)
    g_slice_free1 (total_pixels * sizeof (gfloat), inten_buf);

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
                                 "categories" , "artistic",
                                 "name"       , "gegl:oilify",
                                 "description", _("Emulate an oil painting"),
                                 NULL);
}
#endif
