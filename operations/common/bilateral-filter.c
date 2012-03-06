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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES


gegl_chant_double_ui (blur_radius, _("Blur radius"), 0.0, 1000.0, 4.0, 0.0, 100.0, 1.5,
  _("Radius of square pixel region, (width and height will be radius*2+1)."))
gegl_chant_double (edge_preservation, _("Edge preservation"), 0.0, 100.0, 8.0,
  _("Amount of edge preservation"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "bilateral-filter.c"

#include "gegl-chant.h"
#include <math.h>

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gdouble              radius,
                  gdouble              preserve);

#include <stdio.h>

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = ceil (o->blur_radius);
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"#define POW2(a) ((a) * (a))                                           \n"
"kernel void bilateral_filter(global float4 *in,                       \n"
"                             global float4 *out,                      \n"
"                             const  float radius,                     \n"
"                             const  float preserve)                   \n"
"{                                                                     \n"
"    int gidx       = get_global_id(0);                                \n"
"    int gidy       = get_global_id(1);                                \n"
"    int n_radius   = ceil(radius);                                    \n"
"    int dst_width  = get_global_size(0);                              \n"
"    int src_width  = dst_width + n_radius * 2;                        \n"
"                                                                      \n"
"    int u, v, i, j;                                                   \n"
"    float4 center_pix =                                               \n"
"        in[(gidy + n_radius) * src_width + gidx + n_radius];          \n"
"    float4 accumulated = 0.0f;                                        \n"
"    float4 tempf       = 0.0f;                                        \n"
"    float  count       = 0.0f;                                        \n"
"    float  diff_map, gaussian_weight, weight;                         \n"
"                                                                      \n"
"    for (v = -n_radius;v <= n_radius; ++v)                            \n"
"    {                                                                 \n"
"        for (u = -n_radius;u <= n_radius; ++u)                        \n"
"        {                                                             \n"
"            i = gidx + n_radius + u;                                  \n"
"            j = gidy + n_radius + v;                                  \n"
"                                                                      \n"
"            int gid1d = i + j * src_width;                            \n"
"            tempf = in[gid1d];                                        \n"
"                                                                      \n"
"            diff_map = exp (                                          \n"
"                - (   POW2(center_pix.x - tempf.x)                    \n"
"                    + POW2(center_pix.y - tempf.y)                    \n"
"                    + POW2(center_pix.z - tempf.z))                   \n"
"                * preserve);                                          \n"
"                                                                      \n"
"            gaussian_weight =                                         \n"
"                exp( - 0.5f * (POW2(u) + POW2(v)) / radius);          \n"
"                                                                      \n"
"            weight = diff_map * gaussian_weight;                      \n"
"                                                                      \n"
"            accumulated += tempf * weight;                            \n"
"            count += weight;                                          \n"
"        }                                                             \n"
"    }                                                                 \n"
"    out[gidx + gidy * dst_width] = accumulated / count;               \n"
"}                                                                     \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_bilateral_filter (cl_mem                in_tex,
                     cl_mem                out_tex,
                     size_t                global_worksize,
                     const GeglRectangle  *roi,
                     gfloat                radius,
                     gfloat                preserve)
{
  cl_int cl_err = 0;
  size_t global_ws[2];

  if (!cl_data)
  {
    const char *kernel_name[] = {"bilateral_filter", NULL};
    cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
  }

  if (!cl_data) return 1;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_float), (void*)&radius);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&preserve);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  return cl_err;
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

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,   result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format, GEGL_CL_BUFFER_READ, op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_bilateral_filter(i->tex[read][j], i->tex[0][j], i->size[0][j], &i->roi[0][j], ceil(o->blur_radius), o->edge_preservation);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:bilateral-filter: %s", gegl_cl_errstring(cl_err));
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
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;

  if (o->blur_radius >= 1.0 && gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (o->blur_radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      bilateral_filter (input, &compute, output, result, o->blur_radius, o->edge_preservation);
    }

  return  TRUE;
}

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gdouble              radius,
                  gdouble              preserve)
{
  gfloat *gauss;
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint width = (gint) radius * 2 + 1;
  gint iradius = radius;
  gint src_width = src_rect->width;
  gint src_height = src_rect->height;

  gauss = g_newa (gfloat, width * width);
  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset = 0;

#define POW2(a) ((a)*(a))
  for (y=-iradius;y<=iradius;y++)
    for (x=-iradius;x<=iradius;x++)
      {
        gauss[x+(int)radius + (y+(int)radius)*width] = exp(- 0.5*(POW2(x)+POW2(y))/radius   );
      }

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {
        gint u,v;
        gfloat *center_pix = src_buf + ((x+iradius)+((y+iradius) * src_width)) * 4;
        gfloat  accumulated[4]={0,0,0,0};
        gfloat  count=0.0;

        for (v=-iradius;v<=iradius;v++)
          for (u=-iradius;u<=iradius;u++)
            {
              gint i,j;
              i = x + radius + u;
              j = y + radius + v;
              if (i >= 0 && i < src_width &&
                  j >= 0 && j < src_height)
                {
                  gint c;

                  gfloat *src_pix = src_buf + (i + j * src_width) * 4;

                  gfloat diff_map   = exp (- (POW2(center_pix[0] - src_pix[0])+
                                              POW2(center_pix[1] - src_pix[1])+
                                              POW2(center_pix[2] - src_pix[2])) * preserve
                                          );
                  gfloat gaussian_weight;
                  gfloat weight;

                  gaussian_weight = gauss[u+(int)radius+(v+(int)radius)*width];

                  weight = diff_map * gaussian_weight;

                  for (c=0;c<4;c++)
                    {
                      accumulated[c] += src_pix[c] * weight;
                    }
                  count += weight;
                }
            }

        for (u=0; u<4;u++)
          dst_buf[offset*4+u] = accumulated[u]/count;
        offset++;
      }
  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
           "name", "gegl:bilateral-filter",
           "categories", "misc",
           "description",
           _("An edge preserving blur filter that can be used for noise reduction. "
          "It is a gaussian blur where the contribution of neighbourhood pixels "
          "are weighted by the color difference from the center pixel."),
           NULL);
}

#endif
