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
 */

/*
 * Copyright 2011 Victor Oliveira <victormatheus@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (horizontal,  _("Horizontal"), TRUE)

property_boolean (vertical,  _("Vertical"), TRUE)

property_boolean (keep_signal,  _("Keep Signal"), TRUE)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_FILE "edge-sobel.c"

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define SOBEL_RADIUS 1

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal,
            gboolean            has_alpha);



static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  //GeglProperties              *o = GEGL_PROPERTIES (operation);

  const Babl *source_format = gegl_operation_get_source_format (operation, "input");

  area->left = area->right = area->top = area->bottom = SOBEL_RADIUS;

  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));

  if (source_format && !babl_format_has_alpha (source_format))
    gegl_operation_set_format (operation, "output", babl_format ("RGB float"));
  else
    gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/edge-sobel.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_edge_sobel (cl_mem              in_tex,
               cl_mem              out_tex,
               size_t              global_worksize,
               const GeglRectangle *roi,
               gboolean            horizontal,
               gboolean            vertical,
               gboolean            keep_signal,
               gboolean            has_alpha)
{
  const size_t gbl_size[2] = {roi->width, roi->height};
  cl_int n_horizontal  = horizontal;
  cl_int n_vertical    = vertical;
  cl_int n_keep_signal = keep_signal;
  cl_int n_has_alpha   = has_alpha;
  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_edgesobel", NULL};
      cl_data = gegl_cl_compile_and_build (edge_sobel_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem), (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int), (void*)&n_horizontal);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int), (void*)&n_vertical);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int), (void*)&n_keep_signal);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_int), (void*)&n_has_alpha);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(),
                                       cl_data->kernel[0], 2,
                                       NULL, gbl_size, NULL,
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
            const GeglRectangle *result,
            gboolean             has_alpha)
{
  const Babl *in_format  = babl_format ("RGBA float");
  const Babl *out_format = babl_format ("RGBA float");
  gint err;

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
                                             GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;

      err = cl_edge_sobel(i->tex[read],
                          i->tex[0],
                          i->size[0],
                          &i->roi[0],
                          o->horizontal,
                          o->vertical,
                          o->keep_signal,
                          has_alpha);

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
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle compute;
  gboolean has_alpha;

  compute = gegl_operation_get_required_for_output (operation, "input", result);
  has_alpha = babl_format_has_alpha (gegl_operation_get_format (operation, "output"));

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result, has_alpha))
      return TRUE;

  edge_sobel (input, &compute, output, result, o->horizontal, o->vertical, o->keep_signal, has_alpha);
  return TRUE;
}

inline static gfloat
RMS(gfloat a, gfloat b)
{
  return sqrtf(a*a+b*b);
}

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal,
            gboolean            has_alpha)
{

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  gint src_width = src_rect->width;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {

        gfloat hor_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat ver_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gfloat *center_pix = src_buf + ((x+SOBEL_RADIUS)+((y+SOBEL_RADIUS) * src_width)) * 4;

        gint c;

        if (horizontal)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                hor_grad[c] +=
                    -1.0f*src_pix[c-4-src_width*4]+ src_pix[c+4-src_width*4] +
                    -2.0f*src_pix[c-4] + 2.0f*src_pix[c+4] +
                    -1.0f*src_pix[c-4+src_width*4]+ src_pix[c+4+src_width*4];
          }

        if (vertical)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                ver_grad[c] +=
                  -1.0f*src_pix[c-4-src_width*4]-2.0f*src_pix[c-src_width*4]-1.0f*src_pix[c+4-src_width*4] +
                  src_pix[c-4+src_width*4]+2.0f*src_pix[c+src_width*4]+     src_pix[c+4+src_width*4];
        }

        if (horizontal && vertical)
          {
            for (c=0;c<3;c++)
              // normalization to [0, 1]
              gradient[c] = RMS(hor_grad[c],ver_grad[c])/1.41f;
          }
        else
          {
            if (keep_signal)
              {
                for (c=0;c<3;c++)
                  gradient[c] = hor_grad[c]+ver_grad[c];
              }
            else
              {
                for (c=0;c<3;c++)
                  gradient[c] = fabsf(hor_grad[c]+ver_grad[c]);
              }
          }

        if (has_alpha)
          gradient[3] = center_pix[3];
        else
          gradient[3] = 1.0f;


        for (c=0; c<4;c++)
          dst_buf[offset*4+c] = gradient[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = TRUE;

  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:edge-sobel",
    "categories",  "edge-detect",
    "description", _("Specialized direction-dependent edge detection"),
          NULL);
}

#endif
