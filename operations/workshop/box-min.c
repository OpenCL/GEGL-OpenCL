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

gegl_chant_double (radius, _("Radius"), 0.0, 200.0, 4.0,
  _("Radius of square pixel region (width and height will be radius*2+1)"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "box-min.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

static inline gfloat
get_min_component (gfloat *buf,
                   gint    buf_width,
                   gint    buf_height,
                   gint    x0,
                   gint    y0,
                   gint    width,
                   gint    height,
                   gint    component)
{
  gint    x, y;
  gfloat min=1000000000.0;
  gint    count=0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            if (buf [offset] < min)
              min = buf[offset];
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
   return min;
}

static void
hor_min (GeglBuffer          *src,
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

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset = 0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint i;

        for (i=0; i<4; i++)
          dst_buf [offset++] = get_min_component (src_buf,
                               src_rect->width,
                               src_rect->height,
                               u + radius - radius,
                               v + radius,
                               1 + radius*2,
                               1,
                               i);
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
ver_min (GeglBuffer *src,
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
  dst_buf = g_new0 (gfloat, dst_rect->width * src_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset=0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint c;

        for (c=0; c<4; c++)
          dst_buf [offset++] =
           get_min_component (src_buf,
                              src_rect->width,
                              src_rect->height,
                              u,
                              v - radius,
                              1,
                              1 + radius * 2,
                              c);
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left  =
  area->right =
  area->top   =
  area->bottom = GEGL_CHANT_PROPERTIES (operation)->radius;
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/box-min.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_box_min (cl_mem                in_tex,
            cl_mem                aux_tex,
            cl_mem                out_tex,
            size_t                global_worksize,
            const GeglRectangle  *roi,
            gint                  radius)
{
  cl_int cl_err = 0;
  size_t global_ws_hor[2], global_ws_ver[2];
  size_t local_ws_hor[2], local_ws_ver[2];

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_min_hor", "kernel_min_ver", NULL};
      cl_data = gegl_cl_compile_and_build (box_min_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  local_ws_hor[0] = 1;
  local_ws_hor[1] = 256;
  global_ws_hor[0] = roi->height + 2 * radius;
  global_ws_hor[1] = ((roi->width + local_ws_hor[1] -1)/local_ws_hor[1]) * local_ws_hor[1];

  local_ws_ver[0] = 1;
  local_ws_ver[1] = 256;
  global_ws_ver[0] = roi->height;
  global_ws_ver[1] = ((roi->width + local_ws_ver[1] -1)/local_ws_ver[1]) * local_ws_ver[1];

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&roi->width);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int),   (void*)&radius);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws_hor, local_ws_hor,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),   (void*)&aux_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_int),   (void*)&roi->width);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 3, sizeof(cl_int),   (void*)&radius);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 2,
                                        NULL, global_ws_ver, local_ws_ver,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
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

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  gint aux  = gegl_buffer_cl_iterator_add_2 (i,
                                             NULL,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_AUX,
                                             0,
                                             0,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;

      err = cl_box_min(i->tex[read],
                       i->tex[aux],
                       i->tex[0],
                       i->size[0],
                       &i->roi[0],
                       ceil (o->radius));

      if (err) return FALSE;
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
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle input_rect = gegl_operation_get_required_for_output (operation, "input", result);

  if (gegl_cl_is_accelerated ())
    {
      if (cl_process (operation, input, output, result))
        return TRUE;
      else
        gegl_cl_disable();
    }

  hor_mim ( input, &input_rect, output, result, o->radius);
  ver_mim (output,      result, output, result, o->radius);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:box-min",
  "categories"  , "misc",
  "description" ,
        _("Sets the target pixel to the value of the minimum value in a box surrounding the pixel"),
        NULL);
}


#endif
