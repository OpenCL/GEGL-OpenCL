/* This file is part of GEGL
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
 * Copyright 2006 Øyvind Kolås
 */


#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation-point-filter.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-node.h"
#include "gegl-utils.h"
#include <string.h>

#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"

#include "opencl/gegl-cl.h"

static gboolean gegl_operation_point_filter_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *output,
                               const GeglRectangle *result);

static gboolean gegl_operation_point_filter_op_process
                              (GeglOperation       *operation,
                               GeglOperationContext *context,
                               const gchar          *output_pad,
                               const GeglRectangle  *roi);

G_DEFINE_TYPE (GeglOperationPointFilter, gegl_operation_point_filter, GEGL_TYPE_OPERATION_FILTER)

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
gegl_operation_point_filter_class_init (GeglOperationPointFilterClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process = gegl_operation_point_filter_op_process;
  operation_class->prepare = prepare;
  operation_class->no_cache = TRUE;

  klass->process = NULL;
  klass->cl_process = NULL;
}

static void
gegl_operation_point_filter_init (GeglOperationPointFilter *self)
{
}

static gboolean
gegl_operation_point_filter_cl_process_tiled (GeglOperation       *operation,
                                              GeglBuffer          *input,
                                              GeglBuffer          *output,
                                              const GeglRectangle *result)
{
  GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  const gint bpp = babl_format_get_bytes_per_pixel (babl_format ("RGBA float"));

  int y, x;
  int errcode;
  cl_mem in_tex = NULL, out_tex = NULL;
  cl_image_format format;

  gfloat* in_data  = (gfloat*) gegl_malloc(result->width * result->height * bpp);
  gfloat* out_data = (gfloat*) gegl_malloc(result->width * result->height * bpp);

  if (in_data == NULL || out_data == NULL) goto error;

  /* un-tile */
  gegl_buffer_get (input, 1.0, result, babl_format ("RGBA float"), in_data, GEGL_AUTO_ROWSTRIDE);

  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;

  in_tex  = gegl_clCreateImage2D (gegl_cl_get_context(),
                                  CL_MEM_READ_ONLY,
                                  &format,
                                  cl_state.max_image_width,
                                  cl_state.max_image_height,
                                  0,  NULL, &errcode);

  if (errcode != CL_SUCCESS) goto error;

  out_tex = gegl_clCreateImage2D (gegl_cl_get_context(),
                                  CL_MEM_WRITE_ONLY,
                                  &format,
                                  cl_state.max_image_width,
                                  cl_state.max_image_height,
                                  0,  NULL, &errcode);

  if (errcode != CL_SUCCESS) goto error;

  for (y=0; y < result->height; y += cl_state.max_image_height)
    for (x=0; x < result->width;  x += cl_state.max_image_width)
      {
        const size_t offset = y * (4 * result->width) + (4 * x);
        const size_t origin[3] = {0, 0, 0};
        const size_t region[3] = {MIN(cl_state.max_image_width,  result->width -x),
                                  MIN(cl_state.max_image_height, result->height-y),
                                  1};
        const size_t global_worksize[2] = {region[0], region[1]};

        GeglRectangle roi = {x, y, region[0], region[1]};

        /* CPU -> GPU */
        errcode = gegl_clEnqueueWriteImage(gegl_cl_get_command_queue(), in_tex, CL_FALSE,
                                           origin, region, result->width * 4 * sizeof(gfloat), 0, &in_data[offset],
                                           0, NULL, NULL);
        if (errcode != CL_SUCCESS) goto error;

        /* Wait */
        errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
        if (errcode != CL_SUCCESS) goto error;

        /* Process */
        errcode = point_filter_class->cl_process(operation, in_tex, out_tex, global_worksize, &roi);
        if (errcode != CL_SUCCESS) goto error;

        /* Wait */
        errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
        if (errcode != CL_SUCCESS) goto error;

        /* GPU -> CPU */
        errcode = gegl_clEnqueueReadImage(gegl_cl_get_command_queue(), out_tex, CL_FALSE,
                                           origin, region, result->width * 4 * sizeof(gfloat), 0, &out_data[offset],
                                           0, NULL, NULL);
        if (errcode != CL_SUCCESS) goto error;

        /* Wait */
        errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
        if (errcode != CL_SUCCESS) goto error;
      }

  errcode = gegl_clFinish(gegl_cl_get_command_queue());
  if (errcode != CL_SUCCESS) goto error;

  /* tile-ize */
  gegl_buffer_set (output, result, babl_format ("RGBA float"), out_data, GEGL_AUTO_ROWSTRIDE);

  gegl_clReleaseMemObject (in_tex);
  gegl_clReleaseMemObject (out_tex);

  gegl_free(in_data);
  gegl_free(out_data);

  return TRUE;

error:
  g_warning("[OpenCL] Error: %s", gegl_cl_errstring(errcode));
  if (in_tex)   gegl_clReleaseMemObject (in_tex);
  if (out_tex)  gegl_clReleaseMemObject (out_tex);
  if (in_data)  free (in_data);
  if (out_data) free (out_data);

  return FALSE;
}

struct buf_tex
{
  GeglBuffer *buf;
  GeglRectangle *region;
  cl_mem *tex;
};

static gboolean
gegl_operation_point_filter_cl_process_full (GeglOperation       *operation,
                                             GeglBuffer          *input,
                                             GeglBuffer          *output,
                                             const GeglRectangle *result)
{
  GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  const gint bpp = babl_format_get_bytes_per_pixel (babl_format ("RGBA float"));

  int y, x, i;
  int errcode;

  gfloat** in_data  = NULL;
  gfloat** out_data = NULL;

  int ntex = 0;
  struct buf_tex input_tex;
  struct buf_tex output_tex;

  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;

  for (y=0; y < result->height; y += cl_state.max_image_height)
   for (x=0; x < result->width;  x += cl_state.max_image_width)
     ntex++;

  input_tex.region  = (GeglRectangle *) gegl_malloc(ntex * sizeof(GeglRectangle));
  output_tex.region = (GeglRectangle *) gegl_malloc(ntex * sizeof(GeglRectangle));
  input_tex.tex     = (cl_mem *)        gegl_malloc(ntex * sizeof(cl_mem));
  output_tex.tex    = (cl_mem *)        gegl_malloc(ntex * sizeof(cl_mem));

  if (input_tex.region == NULL || output_tex.region == NULL || input_tex.tex == NULL || output_tex.tex == NULL)
    goto error;

  size_t *pitch = (size_t *) gegl_malloc(ntex * sizeof(size_t *));
  in_data  = (gfloat**) gegl_malloc(ntex * sizeof(gfloat *));
  out_data = (gfloat**) gegl_malloc(ntex * sizeof(gfloat *));

  if (pitch == NULL || in_data == NULL || out_data == NULL) goto error;

  i = 0;
  for (y=0; y < result->height; y += cl_state.max_image_height)
    for (x=0; x < result->width;  x += cl_state.max_image_width)
      {
        const size_t region[3] = {MIN(cl_state.max_image_width,  result->width -x),
                                  MIN(cl_state.max_image_height, result->height-y)};

        GeglRectangle r = {x, y, region[0], region[1]};
        input_tex.region[i] = output_tex.region[i] = r;

        input_tex.tex[i]  = gegl_clCreateImage2D (gegl_cl_get_context(), CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, &format, region[0], region[1],
                                                  0,  NULL, &errcode);
        if (errcode != CL_SUCCESS) goto error;

        output_tex.tex[i] = gegl_clCreateImage2D (gegl_cl_get_context(), CL_MEM_WRITE_ONLY, &format, region[0], region[1],
                                                  0,  NULL, &errcode);
        if (errcode != CL_SUCCESS) goto error;

        out_data[i] = (gfloat *) gegl_malloc(region[0] * region[1] * bpp);
        if (out_data[i] == NULL) goto error;

        i++;
      }

  for (i=0; i < ntex; i++)
    {
      const size_t origin[3] = {0, 0, 0};
      const size_t region[3] = {input_tex.region[i].width, input_tex.region[i].height, 1};

      /* pre-pinned memory */
      in_data[i]  = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), input_tex.tex[i], CL_TRUE,
                                           CL_MAP_WRITE,
                                           origin, region, &pitch[i], NULL,
                                           0, NULL, NULL, &errcode);
      if (errcode != CL_SUCCESS) goto error;

      /* un-tile */
      gegl_buffer_get (input, 1.0, &input_tex.region[i], babl_format ("RGBA float"), in_data[i], GEGL_AUTO_ROWSTRIDE);
    }

  /* CPU -> GPU */
  for (i=0; i < ntex; i++)
    {
      errcode = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), input_tex.tex[i], in_data[i],
                                              0, NULL, NULL);
      if (errcode != CL_SUCCESS) goto error;
    }

  errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
  if (errcode != CL_SUCCESS) goto error;

  /* Process */
  for (i=0; i < ntex; i++)
    {
      const size_t origin[3] = {0, 0, 0};
      const size_t region[3] = {input_tex.region[i].width, input_tex.region[i].height, 1};
      const size_t global_worksize[2] = {region[0], region[1]};

      errcode = point_filter_class->cl_process(operation, input_tex.tex[i], output_tex.tex[i], global_worksize, &input_tex.region[i]);
      if (errcode != CL_SUCCESS) goto error;
    }

  /* Wait Processing */
  errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
  if (errcode != CL_SUCCESS) goto error;

  /* GPU -> CPU */
  for (i=0; i < ntex; i++)
    {
      const size_t origin[3] = {0, 0, 0};
      const size_t region[3] = {input_tex.region[i].width, input_tex.region[i].height, 1};

      errcode = gegl_clEnqueueReadImage(gegl_cl_get_command_queue(), output_tex.tex[i], CL_FALSE,
                                         origin, region, pitch[i], 0, out_data[i],
                                         0, NULL, NULL);
      if (errcode != CL_SUCCESS) goto error;
    }

  /* Wait */
  errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
  if (errcode != CL_SUCCESS) goto error;

  /* Run! */
  errcode = gegl_clFinish(gegl_cl_get_command_queue());
  if (errcode != CL_SUCCESS) goto error;

  for (i=0; i < ntex; i++)
    {
      /* tile-ize */
      gegl_buffer_set (output, &output_tex.region[i], babl_format ("RGBA float"), out_data[i], GEGL_AUTO_ROWSTRIDE);
    }

  for (i=0; i < ntex; i++)
    {
      gegl_clReleaseMemObject (input_tex.tex[i]);
      gegl_clReleaseMemObject (output_tex.tex[i]);
      gegl_free(out_data[i]);
    }

  gegl_free(input_tex.tex);
  gegl_free(output_tex.tex);
  gegl_free(input_tex.region);
  gegl_free(output_tex.region);
  gegl_free(pitch);

  return TRUE;

error:
  g_warning("[OpenCL] Error: %s", gegl_cl_errstring(errcode));

    for (i=0; i < ntex; i++)
      {
        if (input_tex.tex[i])  gegl_clReleaseMemObject (input_tex.tex[i]);
        if (output_tex.tex[i]) gegl_clReleaseMemObject (output_tex.tex[i]);
        if (out_data[i])       gegl_free(out_data[i]);
      }

  if (input_tex.tex)     gegl_free(input_tex.tex);
  if (output_tex.tex)    gegl_free(output_tex.tex);
  if (input_tex.region)  gegl_free(input_tex.region);
  if (output_tex.region) gegl_free(output_tex.region);
  if (pitch)             gegl_free(pitch);

  return FALSE;
}


static gboolean
gegl_operation_point_filter_process (GeglOperation       *operation,
                                     GeglBuffer          *input,
                                     GeglBuffer          *output,
                                     const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  GeglOperationPointFilterClass *point_filter_class;

  point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  if ((result->width > 0) && (result->height > 0))
    {
      if (cl_state.is_accelerated && point_filter_class->cl_process)
        {
          if (gegl_operation_point_filter_cl_process_full (operation, input, output, result))
            return TRUE;

          /* the function above failed */
          if (gegl_operation_point_filter_cl_process_tiled (operation, input, output, result))
            return TRUE;
        }

      {
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, out_format, GEGL_BUFFER_WRITE);
        gint read = /*output == input ? 0 :*/ gegl_buffer_iterator_add (i, input,  result, in_format, GEGL_BUFFER_READ);
        /* using separate read and write iterators for in-place ideally a single
         * readwrite indice would be sufficient
         */
          while (gegl_buffer_iterator_next (i))
            point_filter_class->process (operation, i->data[read], i->data[0], i->length, &i->roi[0]);
      }
    }
  return TRUE;
}

gboolean gegl_can_do_inplace_processing (GeglOperation       *operation,
                                         GeglBuffer          *input,
                                         const GeglRectangle *result);

gboolean gegl_can_do_inplace_processing (GeglOperation       *operation,
                                         GeglBuffer          *input,
                                         const GeglRectangle *result)
{
  if (!input ||
      GEGL_IS_CACHE (input))
    return FALSE;
  if (gegl_object_get_has_forked (input))
    return FALSE;

  if (input->format == gegl_operation_get_format (operation, "output") &&
      gegl_rectangle_contains (gegl_buffer_get_extent (input), result))
    return TRUE;
  return FALSE;
}


static gboolean gegl_operation_point_filter_op_process
                              (GeglOperation       *operation,
                               GeglOperationContext *context,
                               const gchar          *output_pad,
                               const GeglRectangle  *roi)
{
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  input = gegl_operation_context_get_source (context, "input");

  if (gegl_can_do_inplace_processing (operation, input, roi))
    {
      output = g_object_ref (input);
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));
    }
  else
    {
      output = gegl_operation_context_get_target (context, "output");
    }

  success = gegl_operation_point_filter_process (operation, input, output, roi);
  if (output == GEGL_BUFFER (operation->node->cache))
    gegl_cache_computed (operation->node->cache, roi);

  if (input != NULL)
    g_object_unref (input);
  return success;
}
