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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (radius, _("Radius"), 0.0, 1000.0, 4.0, 0.0, 100.0, 1.5,
   _("Radius of square pixel region, (width and height will be radius*2+1)"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "box-blur.c"

#include "gegl-chant.h"
#include "gegl/gegl-debug.h"
#include <stdio.h>
#include <math.h>

#ifdef USE_DEAD_CODE
static inline float
get_mean_component (gfloat *buf,
                    gint    buf_width,
                    gint    buf_height,
                    gint    x0,
                    gint    y0,
                    gint    width,
                    gint    height,
                    gint    component)
{
  gint    x, y;
  gdouble acc=0;
  gint    count=0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            acc += buf [offset];
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
   if (count)
     return acc/count;
   return 0.0;
}
#endif

static inline void
get_mean_components (gfloat *buf,
                     gint    buf_width,
                     gint    buf_height,
                     gint    x0,
                     gint    y0,
                     gint    width,
                     gint    height,
                     gfloat *components)
{
  gint    y;
  gdouble acc[4]={0,0,0,0};
  gint    count[4]={0,0,0,0};

  gint offset = (y0 * buf_width + x0) * 4;

  for (y=y0; y<y0+height; y++)
    {
    gint x;
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            gint c;
            for (c=0;c<4;c++)
              {
                acc[c] += buf [offset+c];
                count[c]++;
              }
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
    {
      gint c;
      for (c=0;c<4;c++)
        {
         if (count[c])
           components[c] = acc[c]/count[c];
         else
           components[c] = 0.0;
        }
    }
}

/* expects src and dst buf to have the same extent */
static void
hor_blur (GeglBuffer          *src,
          const GeglRectangle *src_rect,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gint                 radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  /* src == dst for hor blur */
  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset = 0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint i;
        gfloat components[4];

        get_mean_components (src_buf,
                             src_rect->width,
                             src_rect->height,
                             u - radius,
                             v,
                             1 + radius*2,
                             1,
                             components);

        for (i=0; i<4; i++)
          dst_buf [offset++] = components[i];
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RaGaBaA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


/* expects dst buf to be radius smaller than src buf */
static void
ver_blur (GeglBuffer          *src,
          const GeglRectangle *src_rect,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gint                 radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset=0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gfloat components[4];
        gint c;

        get_mean_components (src_buf,
                             src_rect->width,
                             src_rect->height,
                             u + radius,  /* 1x radius is the offset between the bufs */
                             v - radius + radius, /* 1x radius is the offset between the bufs */
                             1,
                             1 + radius * 2,
                             components);

        for (c=0; c<4; c++)
          dst_buf [offset++] = components[c];
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RaGaBaA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = ceil (o->radius);

  gegl_operation_set_format (operation, "input",  babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"__kernel void kernel_blur_hor (__global const float4     *in,                                      \n"
"                               __global       float4     *aux,                                     \n"
"                               int width, int radius)                                              \n"
"{                                                                                                  \n"
"  const int in_index = get_global_id(0) * (width + 2 * radius)                                     \n"
"                       + (radius + get_global_id (1));                                             \n"
"                                                                                                   \n"
"  const int aux_index = get_global_id(0) * width + get_global_id (1);                              \n"
"  int i;                                                                                           \n"
"  float4 mean;                                                                                     \n"
"                                                                                                   \n"
"  mean = (float4)(0.0f);                                                                           \n"
"                                                                                                   \n"
"  for (i=-radius; i <= radius; i++)                                                                \n"
"   {                                                                                               \n"
"     mean += in[in_index + i];                                                                     \n"
"   }                                                                                               \n"
"                                                                                                   \n"
"  aux[aux_index] = mean / (2 * radius + 1);                                                        \n"
"}                                                                                                  \n"

"__kernel void kernel_blur_ver (__global const float4     *aux,                                     \n"
"                               __global       float4     *out,                                     \n"
"                               int width, int radius)                                              \n"
"{                                                                                                  \n"
"  const int aux_index = (radius + get_global_id(0)) * width + get_global_id (1);                   \n"
"                                                                                                   \n"
"  const int out_index = get_global_id(0) * width + get_global_id (1);                              \n"
"  int i;                                                                                           \n"
"  float4 mean;                                                                                     \n"
"                                                                                                   \n"
"  mean = (float4)(0.0f);                                                                           \n"
"                                                                                                   \n"
"  for (i=-radius; i <= radius; i++)                                                                \n"
"   {                                                                                               \n"
"     mean += aux[aux_index + i * width];                                                           \n"
"   }                                                                                               \n"
"                                                                                                   \n"
"  out[out_index] = mean / (2 * radius + 1);                                                        \n"
"}                                                                                                  \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_box_blur (cl_mem                in_tex,
             cl_mem                aux_tex,
             cl_mem                out_tex,
             size_t                global_worksize,
             const GeglRectangle  *roi,
             gint                  radius)
{
  cl_int cl_err = 0;
  size_t global_ws_hor[2], global_ws_ver[2];

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_blur_hor", "kernel_blur_ver", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }

  if (!cl_data) return 1;

  global_ws_hor[0] = roi->height + 2 * radius;
  global_ws_hor[1] = roi->width;

  global_ws_ver[0] = roi->height;
  global_ws_ver[1] = roi->width;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&roi->width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int),   (void*)&radius);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws_hor, NULL,
                                        0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  gegl_clEnqueueBarrier (gegl_cl_get_command_queue ());

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),   (void*)&aux_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_int),   (void*)&roi->width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 3, sizeof(cl_int),   (void*)&radius);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 2,
                                        NULL, global_ws_ver, NULL,
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

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format,  GEGL_CL_BUFFER_READ, op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
                gint aux  = gegl_buffer_cl_iterator_add_2 (i, NULL, result, in_format,  GEGL_CL_BUFFER_AUX, 0, 0, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;
      for (j=0; j < i->n; j++)
        {
          cl_err = cl_box_blur(i->tex[read][j], i->tex[aux][j], i->tex[0][j], i->size[0][j], &i->roi[0][j], ceil (o->radius));
          if (cl_err != CL_SUCCESS)
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in gegl:box-blur: %s", gegl_cl_errstring(cl_err));
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
  GeglRectangle rect;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer *temp;
  GeglOperationAreaFilter *op_area;
  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

  rect = *result;

  rect.x-=op_area->left;
  rect.y-=op_area->top;
  rect.width+=op_area->left + op_area->right;
  rect.height+=op_area->top + op_area->bottom;

  temp  = gegl_buffer_new (&rect,
                           babl_format ("RaGaBaA float"));

  hor_blur (input, &rect, temp, &rect, o->radius);
  ver_blur (temp, &rect, output, result, o->radius);

  g_object_unref (temp);
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

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:box-blur",
      "categories",  "blur",
      "description", _("Performs an averaging of a square box of pixels"),
      NULL);
}

#endif
