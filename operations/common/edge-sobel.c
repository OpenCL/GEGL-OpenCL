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

property_boolean (keep_sign,  _("Keep Sign"), TRUE)
     description (_("Keep negative values in result; when off, the absolute value of the result is used instead."))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE edge-sobel.c

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
            gboolean            keep_sign,
            gboolean            has_alpha);



static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  const Babl *source_format = gegl_operation_get_source_format (operation, "input");

  area->left = area->right = area->top = area->bottom = SOBEL_RADIUS;

  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));

  if (source_format && !babl_format_has_alpha (source_format))
    gegl_operation_set_format (operation, "output", babl_format ("RGB float"));
  else
    gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/** FIXME - disabling CL
 *  CL border handling is broken. As is the whole result is shifted to avoid the
 *  issue. This is causing tests to fail. The shift can be corrected, but real
 *  issue is handling pixels that the sobel kernel needs from outside the image.
 */
/*

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
               gboolean            keep_sign,
               gboolean            has_alpha)
{
  const size_t gbl_size[2] = {roi->width, roi->height};
  cl_int n_horizontal  = horizontal;
  cl_int n_vertical    = vertical;
  cl_int n_keep_sign = keep_sign;
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
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int), (void*)&n_keep_sign);
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
                          o->keep_sign,
                          has_alpha);

      if (err) return FALSE;
    }

  return TRUE;
}
*/

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

  /*
  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result, has_alpha))
      return TRUE;
   */

  edge_sobel (input, &compute, output, result,
              o->horizontal, o->vertical, o->keep_sign, has_alpha);
  return TRUE;
}

inline static gfloat
magnitude(gfloat a, gfloat b)
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
            gboolean            keep_sign,
            gboolean            has_alpha)
{
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *after_src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  after_src_buf = src_buf + src_rect->width * src_rect->height * 4;

  /* Apply the Sobel operator. Technically, the following is not Sobel
     as it does not use pixel intensities, but works on the individual
     RGB channels. But it is similar to the classic GIMP and PhotoShop
     filter.

     See paper "History and Definition of the Sobel Operator" by Irwin
     Sobel that seems to be the only free and authentic description.
  */
  offset = 0;
  for (y = 0; y < dst_rect->height; y++)
    for (x = 0; x < dst_rect->width; x++)
      {
        gfloat hor_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat ver_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        gfloat *row_start, *row_next;
        gfloat *tl_px, *t_px, *tr_px;
        gfloat *l_px, *center_px, *r_px;
        gfloat *bl_px, *b_px, *br_px;
        gint c;

        row_start = src_buf + y * src_rect->width * 4;
        row_next = row_start + src_rect->width * 4;

        /* This pixel */
        center_px = row_start + x * 4;
        /* Top pixel */
        t_px = center_px - src_rect->width * 4;
        /* Top-left pixel */
        tl_px = t_px - 4;
        /* Top-right pixel */
        tr_px = t_px + 4;
        /* Left pixel */
        l_px = center_px - 4;
        /* Right pixel */
        r_px = center_px + 4;
        /* Bottom pixel */
        b_px = center_px + src_rect->width * 4;
        /* Bottom-left pixel */
        bl_px = b_px - 4;
        /* Bottom-right pixel */
        br_px = b_px + 4;

        /* If pixels around the center pixel are out of source rect
           bounds, clamp them into the rect. */
        if (t_px < src_buf)
          {
            tl_px += src_rect->width * 4;
            t_px += src_rect->width * 4;
            tr_px += src_rect->width * 4;
          }
        else if (b_px >= after_src_buf)
          {
            bl_px -= src_rect->width * 4;
            b_px -= src_rect->width * 4;
            br_px -= src_rect->width * 4;
          }

        if (l_px < row_start)
          {
            tl_px += 4;
            l_px += 4;
            bl_px += 4;
          }
        else if (r_px >= row_next)
          {
            tr_px -= 4;
            r_px -= 4;
            br_px -= 4;
          }

        if (horizontal)
          {
            /*
             * Horizontal kernel:
             *
             *      [-1  0  +1]
             * Gx = [-2  0  +2] * P
             *      [-1  0  -1]
             */
            for (c = 0; c < 3; c++)
              {
                hor_grad[c] += (-1.0f * tl_px[c]) + (0.0f * t_px[c]) + (1.0f * tr_px[c]);
                hor_grad[c] += (-2.0f * l_px[c]) + (0.0f * center_px[c]) + (2.0f * r_px[c]);
                hor_grad[c] += (-1.0f * bl_px[c]) + (0.0f * b_px[c]) + (1.0f * br_px[c]);
              }
          }

        if (vertical)
          {
            /*
             * Vertical kernel:
             *
             *      [+1  +2  +1]
             * Gy = [ 0   0   0] * P
             *      [-1  -2  -1]
             */
            for (c = 0; c < 3; c++)
              {
                ver_grad[c] += (1.0f * tl_px[c]) + (2.0f * t_px[c]) + (1.0f * tr_px[c]);
                ver_grad[c] += (0.0f * l_px[c]) + (0.0f * center_px[c]) + (0.0f * r_px[c]);
                ver_grad[c] += (-1.0f * bl_px[c]) + (-2.0f * b_px[c]) + (-1.0f * br_px[c]);
              }
          }

        if (horizontal && vertical)
          {
            for (c = 0; c < 3; c++)
              gradient[c] = magnitude (hor_grad[c], ver_grad[c]) / sqrtf (32.0f);
          }
        else
          {
            if (keep_sign)
              {
                for (c = 0; c < 3; c++)
                  gradient[c] = 0.5f + (hor_grad[c] + ver_grad[c]) / 8.0f;
              }
            else
              {
                for (c = 0; c < 3; c++)
                  gradient[c] = fabsf (hor_grad[c] + ver_grad[c]) / 4.0f;
              }
          }

        if (has_alpha)
          gradient[3] = center_px[3];
        else
          gradient[3] = 1.0f;

        for (c = 0; c < 4; c++)
          dst_buf[offset * 4 + c] = gradient[c];

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
    "title",       _("Sobel Edge Detection"),
    "categories",  "edge-detect",
    "description", _("Specialized direction-dependent edge detection"),
          NULL);
}

#endif
