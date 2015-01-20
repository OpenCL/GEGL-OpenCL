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
 * Copyright 1997 Thorsten Schnier <thorsten@arch.usyd.edu.au>
 * Copyright 2011 Victor Oliveira <victormatheus@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE edge-laplace.c

#include "gegl-op.h"
#include <math.h>

#define LAPLACE_RADIUS 2
#define CHUNK_SIZE     1024
#define SQR(x)         ((x)*(x))

static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gfloat              *src_buf,
              gfloat              *temp_buf,
              gfloat              *dst_buf);

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  area->left = area->right = area->top = area->bottom = LAPLACE_RADIUS;

  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  gint    i, j;
  gfloat *buf1, *buf2, *buf3;

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result))
      return TRUE;

  buf1 = g_new (gfloat, SQR (CHUNK_SIZE + LAPLACE_RADIUS * 2) * 4);
  buf2 = g_new (gfloat, SQR (CHUNK_SIZE + LAPLACE_RADIUS * 2) * 4);
  buf3 = g_new (gfloat, SQR (CHUNK_SIZE) * 4);

  for (j = 0; (j-1) * CHUNK_SIZE < result->height; j++)
    for (i = 0; (i-1) * CHUNK_SIZE < result->width; i++)
      {
        GeglRectangle chunked_result;
        GeglRectangle compute;

        chunked_result = *GEGL_RECTANGLE (result->x + i * CHUNK_SIZE,
                                          result->y + j * CHUNK_SIZE,
                                          CHUNK_SIZE, CHUNK_SIZE);

        gegl_rectangle_intersect (&chunked_result, &chunked_result, result);

        if (chunked_result.width < 1  || chunked_result.height < 1)
          continue;

        compute = gegl_operation_get_required_for_output (operation,
                                                          "input",
                                                          &chunked_result);

        edge_laplace (input, &compute, output, &chunked_result,
                      buf1, buf2, buf3);
      }

  g_free (buf1);
  g_free (buf2);
  g_free (buf3);

  return  TRUE;
}

static void
minmax  (gfloat  x1,
         gfloat  x2,
         gfloat  x3,
         gfloat  x4,
         gfloat  x5,
         gfloat *min_result,
         gfloat *max_result)
{
  gfloat min1, min2, max1, max2;

  if (x1 > x2)
    {
      max1 = x1;
      min1 = x2;
    }
  else
    {
      max1 = x2;
      min1 = x1;
    }

  if (x3 > x4)
    {
      max2 = x3;
      min2 = x4;
    }
  else
    {
      max2 = x4;
      min2 = x3;
    }

  if (min1 < min2)
    *min_result = fminf (min1, x5);
  else
    *min_result = fminf (min2, x5);

  if (max1 > max2)
    *max_result = fmaxf (max1, x5);
  else
    *max_result = fmaxf (max2, x5);
}


static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gfloat              *src_buf,
              gfloat              *temp_buf,
              gfloat              *dst_buf)
{

  gint    x, y;
  gint    offset;
  gint    src_width = src_rect->width;

  gegl_buffer_get (src, src_rect, 1.0,
                   babl_format ("R'G'B'A float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_CLAMP);

  for (y = 0; y < dst_rect->height + LAPLACE_RADIUS; y++)
    for (x = 0; x < dst_rect->width + LAPLACE_RADIUS; x++)
      {
        gfloat *src_pix;
        gfloat  gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        gint    c;
        gfloat  minval, maxval;
        gint    i = x + LAPLACE_RADIUS - 1;
        gint    j = y + LAPLACE_RADIUS - 1;

        offset = i + j * src_width;
        src_pix = src_buf + offset * 4;

        for (c = 0; c < 3; c++)
          {
            gfloat s1 = src_pix[c - src_width * 4];
            gfloat s2 = src_pix[c + src_width * 4];
            gfloat s3 = src_pix[c - 4];
            gfloat s4 = src_pix[c + 4];
            gfloat s  = src_pix[c];
            gfloat temp_value;

            /* four-neighbourhood */
            minmax (s1, s2, s3, s4, s,
                    &minval, &maxval);

            gradient[c] = 0.5f * fmaxf ((maxval - s), (s - minval));

            /* nine-neighbourhood */
            temp_value = (src_pix[c - 4 - src_width * 4] +
                          s1 +
                          src_pix[c + 4 - src_width * 4] +

                          s3 - 8.0f * s + s4 +

                          src_pix[c - 4 + src_width * 4] +
                          s2 +
                          src_pix[c + 4 + src_width * 4]);


            if (temp_value < GEGL_FLOAT_EPSILON)
              gradient[c] *= -1.0f;
          }

        /* alpha */
        gradient[3] = src_pix[3];

        for (c = 0; c < 4; c++)
          temp_buf[offset * 4 + c] = gradient[c];
      }

  /* 1-pixel edges */
  offset = 0;

  for (y = 0; y < dst_rect->height; y++)
    for (x = 0; x < dst_rect->width; x++)
      {
        gfloat  value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        gint    c;
        gint    i = x + LAPLACE_RADIUS;
        gint    j = y + LAPLACE_RADIUS;
        gfloat *src_pix = temp_buf + (i + j * src_width) * 4;

        for (c = 0; c < 3; c++)
          {
            gfloat current = src_pix[c];

            current = ((current > 0.0f) &&
                       (src_pix[c - 4 - src_width * 4] < 0.0f ||
                        src_pix[c + 4 - src_width * 4] < 0.0f ||
                        src_pix[c     - src_width * 4] < 0.0f ||
                        src_pix[c - 4 + src_width * 4] < 0.0f ||
                        src_pix[c + 4 + src_width * 4] < 0.0f ||
                        src_pix[c     + src_width * 4] < 0.0f ||
                        src_pix[c - 4                ] < 0.0f ||
                        src_pix[c + 4                ] < 0.0f)) ?
              current : 0.0f;

            value[c] = current;
          }

        /* alpha */
        value[3] = src_pix[3];

        for (c = 0; c < 4; c++)
          dst_buf[offset * 4 + c] = value[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("R'G'B'A float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/edge-laplace.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_edge_laplace (cl_mem                in_tex,
                 cl_mem                aux_tex,
                 cl_mem                out_tex,
                 const GeglRectangle  *src_rect,
                 const GeglRectangle  *roi,
                 gint                  radius)
{
  cl_int cl_err = 0;
  size_t global_ws_in[2];
  size_t global_ws_aux[2];

  if (!cl_data)
    {
      const char *kernel_name[] = {"pre_edgelaplace", "knl_edgelaplace", NULL};
      cl_data = gegl_cl_compile_and_build (edge_laplace_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  global_ws_in[0] = roi->width  + LAPLACE_RADIUS;
  global_ws_in[1] = roi->height + LAPLACE_RADIUS;

  global_ws_aux[0] = roi->width;
  global_ws_aux[1] = roi->height;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof (cl_mem), &in_tex,
                                    sizeof (cl_mem), &aux_tex,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws_in, NULL,
                                        0, NULL, NULL);
  CL_CHECK;


  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[1],
                                    sizeof (cl_mem), &aux_tex,
                                    sizeof (cl_mem), &out_tex,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 2,
                                        NULL, global_ws_aux, NULL,
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
  gint err = 0;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

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
                                              op_area->left   - 1,
                                              op_area->right  - 1,
                                              op_area->top    - 1,
                                              op_area->bottom - 1);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_edge_laplace (i->tex[read],
                             i->tex[aux],
                             i->tex[0],
                             &i->roi[read],
                             &i->roi[0],
                             LAPLACE_RADIUS);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  return !err;
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
    "name",        "gegl:edge-laplace",
    "title",       _("Laplacian Edge Detection"),
    "categories",  "edge-detect",
    "license",     "GPL3+",
    "description", _("High-resolution edge detection"),
    NULL);
}

#endif
