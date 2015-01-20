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
 *           2012 Pavel Roschin <roshin@scriptumplus.ru>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 4)
   description(_("Radius of square pixel region, (width and height will be radius*2+1)"))
   value_range (0, 1000)
   ui_range    (0, 100)
   ui_gamma   (1.5)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE box-blur.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

#define SRC_OFFSET (row + u + radius * 2) * 4

static void
hor_blur (GeglBuffer          *src,
          const GeglRectangle *src_rect,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gint                 radius)
{
  gint u,v;
  gint i;
  gint offset;
  gint src_offset;
  gint prev_rad = radius * 4 + 4;
  gint next_rad = radius * 4;
  gint row;
  gfloat *src_buf;
  gfloat *dst_buf;
  gfloat rad1 = 1.0 / (gfloat)(radius * 2 + 1);

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  offset = 0;
  for (v = 0; v < dst_rect->height; v++)
    {
      /* here just radius, not radius * 2 as in ver_blur beacuse
       * we enlarged dst_buf by y earlier */
      row = (v + radius) * src_rect->width;
      /* prepare - set first column of pixels */
      for (u = -radius; u <= radius; u++)
        {
          src_offset = SRC_OFFSET;
          for (i = 0; i < 4; i++)
            dst_buf[offset + i] += src_buf[src_offset + i] * rad1;
        }
      offset += 4;
      /* iterate other pixels by moving a window - very fast */
      for (u = 1; u < dst_rect->width; u++)
        {
          src_offset = SRC_OFFSET;
          for (i = 0; i < 4; i++)
          {
            dst_buf[offset] = dst_buf[offset - 4]
                            - src_buf[src_offset - prev_rad] * rad1
                            + src_buf[src_offset + next_rad] * rad1;
            src_offset++;
            offset++;
          }
        }
    }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RaGaBaA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);
}

static void
ver_blur (GeglBuffer          *src,
          const GeglRectangle *src_rect,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gint                 radius)
{
  gint u, v;
  gint i;
  gint offset;
  gint src_offset;
  gint prev_rad = (radius * 4 + 4) * src_rect->width;
  gint next_rad = (radius * 4) * src_rect->width;
  gint row;
  gfloat *src_buf;
  gfloat *dst_buf;
  gfloat rad1 = 1.0 / (gfloat)(radius * 2 + 1);

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  /* prepare: set first row of pixels */
  for (v = -radius; v <= radius; v++)
    {
      row = (v + radius * 2) * src_rect->width;
      for (u = 0; u < dst_rect->width; u++)
        {
          src_offset = SRC_OFFSET;
          for (i = 0; i < 4; i++)
            dst_buf[u * 4 + i] += src_buf[src_offset + i] * rad1;
        }
    }
  /* skip first row */
  offset = dst_rect->width * 4;
  for (v = 1; v < dst_rect->height; v++)
    {
      row = (v + radius * 2) * src_rect->width;
      for (u = 0; u < dst_rect->width; u++)
        {
          src_offset = SRC_OFFSET;
          for (i = 0; i < 4; i++)
          {
            dst_buf[offset] = dst_buf[offset - 4 * dst_rect->width]
                            - src_buf[src_offset - prev_rad] * rad1
                            + src_buf[src_offset + next_rad] * rad1;
            src_offset++;
            offset++;
          }
        }
    }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RaGaBaA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);
}

#undef SRC_OFFSET

static void prepare (GeglOperation *operation)
{
  GeglProperties              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_PROPERTIES (operation);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->radius;

  gegl_operation_set_format (operation, "input",  babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/box-blur.cl.h"
static GeglClRunData *cl_data = NULL;
static gboolean
cl_box_blur (cl_mem                in_tex,
             cl_mem                aux_tex,
             cl_mem                out_tex,
             size_t                global_worksize,
             const GeglRectangle  *roi,
             gint                  radius)
{
  cl_int cl_err = 0;
  size_t global_ws_hor[2], global_ws_ver[2], global_ws[2];
  size_t local_ws_hor[2], local_ws_ver[2], local_ws[2];
  size_t step_size ;
  if (!cl_data)
    {
      const char *kernel_name[] = { "kernel_blur_hor", "kernel_blur_ver","kernel_box_blur_fast", NULL};
      cl_data = gegl_cl_compile_and_build (box_blur_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;
  step_size = 64;
  local_ws[0]=256;
  local_ws[1]=1;


  if( radius <=110 )
  {
    global_ws[0] = (roi->width + local_ws[0] - 2 * radius - 1) / ( local_ws[0] - 2 * radius ) * local_ws[0];
    global_ws[1] = (roi->height + step_size - 1) / step_size;
    cl_err = gegl_cl_set_kernel_args(cl_data->kernel[2],
                                     sizeof(cl_mem), (void *)&in_tex,
                                     sizeof(cl_mem), (void *)&out_tex,
                                     sizeof(cl_float4)*local_ws[0], (void *)NULL,
                                     sizeof(cl_int), (void *)&roi->width,
                                     sizeof(cl_int), (void *)&roi->height,
                                     sizeof(cl_int), (void *)&radius,
                                     sizeof(cl_int), (void *)&step_size, NULL);
    CL_CHECK;
    cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(),
                                         cl_data->kernel[2], 2,
                                         NULL, global_ws, local_ws, 0, NULL, NULL );
       CL_CHECK;

  }
  else
  {
    local_ws_hor[0] = 1;
    local_ws_hor[1] = 256;
    global_ws_hor[0] = roi->height + 2 * radius;
    global_ws_hor[1] = ((roi->width + local_ws_hor[1] -1)/local_ws_hor[1]) * local_ws_hor[1];

    local_ws_ver[0] = 1;
    local_ws_ver[1] = 256;
    global_ws_ver[0] = roi->height;
    global_ws_ver[1] = ((roi->width + local_ws_ver[1] -1)/local_ws_ver[1]) * local_ws_ver[1];


    cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                      sizeof(cl_mem), (void*)&in_tex,
                                      sizeof(cl_mem), (void*)&aux_tex,
                                      sizeof(cl_int), (void*)&roi->width,
                                      sizeof(cl_int), (void*)&radius,
                                      NULL);
    CL_CHECK;
    cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                          cl_data->kernel[0], 2,
                                          NULL, global_ws_hor, local_ws_hor,
                                          0, NULL, NULL);
    CL_CHECK;


    cl_err = gegl_cl_set_kernel_args (cl_data->kernel[1],
                                      sizeof(cl_mem), (void*)&aux_tex,
                                      sizeof(cl_mem), (void*)&out_tex,
                                      sizeof(cl_int), (void*)&roi->width,
                                      sizeof(cl_int), (void*)&radius,
                                      NULL);
    CL_CHECK;

    cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                          cl_data->kernel[1], 2,
                                          NULL, global_ws_ver, local_ws_ver,
                                          0, NULL, NULL);
    CL_CHECK;
  }

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

  gint err = 0;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);

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
                                             GEGL_ABYSS_CLAMP);

  gint aux = gegl_buffer_cl_iterator_add_aux (i,
                                              result,
                                              in_format,
                                              0,
                                              0,
                                              op_area->top,
                                              op_area->bottom);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_box_blur (i->tex[read],
                         i->tex[aux],
                         i->tex[0],
                         i->size[0],
                         &i->roi[0],
                         o->radius);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  return !err;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglRectangle rect;
  GeglRectangle tmprect;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer *temp;
  GeglOperationAreaFilter *op_area;
  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result))
      return TRUE;

  rect = *result;
  tmprect = *result;

  rect.x       -= op_area->left * 2;
  rect.y       -= op_area->top * 2;
  rect.width   += (op_area->left + op_area->right) * 2;
  rect.height  += (op_area->top + op_area->bottom) * 2;
  /* very tricky: enlarge temp buffer to avoid seams in second pass */
  tmprect.y      -= o->radius;
  tmprect.height += o->radius * 2;

  temp  = gegl_buffer_new (&tmprect,
                           babl_format ("RaGaBaA float"));

  /* doing second pass in separate gegl op may be significantly faster */
  hor_blur (input, &rect, temp, &tmprect, o->radius);
  ver_blur (temp, &rect, output, result, o->radius);

  g_object_unref (temp);
  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
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
      "title",       _("Box Blur"),
      "categories",  "blur",
      "description", _("Blur resulting from averaging the colors of a square neighbourhood."),
      NULL);
}

#endif
