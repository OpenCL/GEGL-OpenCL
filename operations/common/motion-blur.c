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
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES


gegl_chant_double_ui (length, _("Length"), 0.0, 1000.0, 10.0, 0.0, 300.0, 1.5,
                     _("Length of blur in pixels"))
gegl_chant_double_ui (angle,  _("Angle"),  -360, 360, 0, -180.0, 180.0, 1.0,
                     _("Angle of blur in degrees"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "motion-blur.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter* op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO* o = GEGL_CHANT_PROPERTIES (operation);

  gdouble theta = o->angle * G_PI / 180.0;
  gdouble offset_x = fabs(o->length * cos(theta));
  gdouble offset_y = fabs(o->length * sin(theta));

  op_area->left   =
  op_area->right  = (gint)ceil(0.5 * offset_x);
  op_area->top    =
  op_area->bottom = (gint)ceil(0.5 * offset_y);

  gegl_operation_set_format (operation, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"int CLAMP(int val,int lo,int hi)                                      \n"
"{                                                                     \n"
"    return (val < lo) ? lo : ((hi < val) ? hi : val);                 \n"
"}                                                                     \n"
"                                                                      \n"
"float4 get_pixel_color_CL(const __global float4 *in_buf,              \n"
"                          int     rect_width,                         \n"
"                          int     rect_height,                        \n"
"                          int     rect_x,                             \n"
"                          int     rect_y,                             \n"
"                          int     x,                                  \n"
"                          int     y)                                  \n"
"{                                                                     \n"
"    int ix = x - rect_x;                                              \n"
"    int iy = y - rect_y;                                              \n"
"                                                                      \n"
"    ix = CLAMP(ix, 0, rect_width-1);                                  \n"
"    iy = CLAMP(iy, 0, rect_height-1);                                 \n"
"                                                                      \n"
"    return in_buf[iy * rect_width + ix];                              \n"
"}                                                                     \n"
"                                                                      \n"
"__kernel void motion_blur_CL(const __global float4 *src_buf,          \n"
"                             int     src_width,                       \n"
"                             int     src_height,                      \n"
"                             int     src_x,                           \n"
"                             int     src_y,                           \n"
"                             __global float4 *dst_buf,                \n"
"                             int     dst_x,                           \n"
"                             int     dst_y,                           \n"
"                             int     num_steps,                       \n"
"                             float   offset_x,                        \n"
"                             float   offset_y)                        \n"
"{                                                                     \n"
"    int gidx = get_global_id(0);                                      \n"
"    int gidy = get_global_id(1);                                      \n"
"                                                                      \n"
"    float4 sum = 0.0f;                                                \n"
"    int px = gidx + dst_x;                                            \n"
"    int py = gidy + dst_y;                                            \n"
"                                                                      \n"
"    for(int step = 0; step < num_steps; ++step)                       \n"
"    {                                                                 \n"
"        float t = num_steps == 1 ? 0.0f :                             \n"
"            step / (float)(num_steps - 1) - 0.5f;                     \n"
"                                                                      \n"
"        float xx = px + t * offset_x;                                 \n"
"        float yy = py + t * offset_y;                                 \n"
"                                                                      \n"
"        int   ix = (int)floor(xx);                                    \n"
"        int   iy = (int)floor(yy);                                    \n"
"                                                                      \n"
"        float dx = xx - floor(xx);                                    \n"
"        float dy = yy - floor(yy);                                    \n"
"                                                                      \n"
"        float4 mixy0,mixy1,pix0,pix1,pix2,pix3;                       \n"
"                                                                      \n"
"        pix0 = get_pixel_color_CL(src_buf, src_width,                 \n"
"            src_height, src_x, src_y, ix,   iy);                      \n"
"        pix1 = get_pixel_color_CL(src_buf, src_width,                 \n"
"            src_height, src_x, src_y, ix+1, iy);                      \n"
"        pix2 = get_pixel_color_CL(src_buf, src_width,                 \n"
"            src_height, src_x, src_y, ix,   iy+1);                    \n"
"        pix3 = get_pixel_color_CL(src_buf, src_width,                 \n"
"            src_height, src_x, src_y, ix+1, iy+1);                    \n"
"                                                                      \n"
"        mixy0 = dy * (pix2 - pix0) + pix0;                            \n"
"        mixy1 = dy * (pix3 - pix1) + pix1;                            \n"
"                                                                      \n"
"        sum  += dx * (mixy1 - mixy0) + mixy0;                         \n"
"    }                                                                 \n"
"                                                                      \n"
"    dst_buf[gidy * get_global_size(0) + gidx] =                       \n"
"        sum / num_steps;                                              \n"
"}                                                                     \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_motion_blur (cl_mem                in_tex,
                cl_mem                out_tex,
                size_t                global_worksize,
                const GeglRectangle  *roi,
                const GeglRectangle  *src_rect,
                gint                  num_steps,
                gfloat                offset_x,
                gfloat                offset_y)
{
  cl_int cl_err = 0;
  size_t global_ws[2];

  if (!cl_data)
  {
    const char *kernel_name[] = {"motion_blur_CL", NULL};
    cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
  }

  if (!cl_data) return 1;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  1, sizeof(cl_int),   (void*)&src_rect->width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  2, sizeof(cl_int),   (void*)&src_rect->height);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  3, sizeof(cl_int),   (void*)&src_rect->x);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  4, sizeof(cl_int),   (void*)&src_rect->y);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  5, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  6, sizeof(cl_int),   (void*)&roi->x);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  7, sizeof(cl_int),   (void*)&roi->y);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  8, sizeof(cl_int),   (void*)&num_steps);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0],  9, sizeof(cl_float), (void*)&offset_x);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 10, sizeof(cl_float), (void*)&offset_y);
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
            const GeglRectangle *result,
            const GeglRectangle *src_rect)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;
  gint j;
  cl_int cl_err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gdouble theta = o->angle * G_PI / 180.0;
  gfloat  offset_x = (gfloat)(o->length * cos(theta));
  gfloat  offset_y = (gfloat)(o->length * sin(theta));
  gint num_steps = (gint)ceil(o->length) + 1;

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,   result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format,  GEGL_CL_BUFFER_READ,
                                                           op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_motion_blur(i->tex[read][j], i->tex[0][j], i->size[0][j], &i->roi[0][j], &i->roi[read][j], num_steps, offset_x, offset_y);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:motion-blur: %s", gegl_cl_errstring(cl_err));
        return FALSE;
      }
    }
  }
  return TRUE;
}

static inline gfloat*
get_pixel_color(gfloat* in_buf,
                const GeglRectangle* rect,
                gint x,
                gint y)
{
  gint ix = x - rect->x;
  gint iy = y - rect->y;
  ix = CLAMP(ix, 0, rect->width-1);
  iy = CLAMP(iy, 0, rect->height-1);

  return &in_buf[(iy*rect->width + ix) * 4];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglRectangle src_rect;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;
  gfloat* in_buf;
  gfloat* out_buf;
  gfloat* out_pixel;
  gint x,y;

  gdouble theta = o->angle * G_PI / 180.0;
  gdouble offset_x = o->length * cos(theta);
  gdouble offset_y = o->length * sin(theta);
  gint num_steps = (gint)ceil(o->length) + 1;
  gfloat inv_num_steps = 1.0f / num_steps;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  src_rect = *roi;
  src_rect.x -= op_area->left;
  src_rect.y -= op_area->top;
  src_rect.width += op_area->left + op_area->right;
  src_rect.height += op_area->top + op_area->bottom;

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, roi, &src_rect))
      return TRUE;

  in_buf = g_new (gfloat, src_rect.width * src_rect.height * 4);
  out_buf = g_new0 (gfloat, roi->width * roi->height * 4);
  out_pixel = out_buf;

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RaGaBaA float"), in_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y=0; y<roi->height; ++y)
    {
      for (x=0; x<roi->width; ++x)
        {
          gint step;
          gint c;
          gint px = x+roi->x;
          gint py = y+roi->y;
          gfloat sum[4] = {0,0,0,0};
          for (step=0; step<num_steps; ++step)
            {
              gdouble t = num_steps == 1 ? 0.0 : step / (gdouble)(num_steps-1) - 0.5;

              /* get the interpolated pixel position for this step */
              gdouble xx = px + t*offset_x;
              gdouble yy = py + t*offset_y;
              gint ix = (gint)floor(xx);
              gint iy = (gint)floor(yy);
              gdouble dx = xx - floor(xx);
              gdouble dy = yy - floor(yy);

              /* do bilinear interpolation to get a nice smooth result */
              gfloat *pix0, *pix1, *pix2, *pix3;
              gfloat mixy0[4];
              gfloat mixy1[4];

              pix0 = get_pixel_color(in_buf, &src_rect, ix, iy);
              pix1 = get_pixel_color(in_buf, &src_rect, ix+1, iy);
              pix2 = get_pixel_color(in_buf, &src_rect, ix, iy+1);
              pix3 = get_pixel_color(in_buf, &src_rect, ix+1, iy+1);
              for (c=0; c<4; ++c)
                {
                  mixy0[c] = dy*(pix2[c] - pix0[c]) + pix0[c];
                  mixy1[c] = dy*(pix3[c] - pix1[c]) + pix1[c];
                  sum[c] += dx*(mixy1[c] - mixy0[c]) + mixy0[c];
                }
            }

          for (c=0; c<4; ++c)
            *out_pixel++ = sum[c] * inv_num_steps;
        }
    }

  gegl_buffer_set (output, roi, 0, babl_format ("RaGaBaA float"), out_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (in_buf);
  g_free (out_buf);


  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationFilterClass      *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:motion-blur",
    "categories" , "blur",
    "description", _("Linear motion blur"),
    NULL);
}

#endif
