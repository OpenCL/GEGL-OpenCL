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

#include "gegl-cl.h"

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
gegl_operation_point_filter_cl_process (GeglOperation       *operation,
                                        GeglBuffer          *input,
                                        GeglBuffer          *output,
                                        const GeglRectangle *result)
{
  GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  const GeglRectangle *ext = gegl_buffer_get_extent (input);
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
        const size_t offset = y * (4 * ext->width) + (4 * x);
        const size_t origin[3] = {0, 0, 0};
        const size_t region[3] = {MIN(cl_state.max_image_width,  result->width -x),
                                  MIN(cl_state.max_image_height, result->height-y),
                                  1};
        const size_t global_worksize[2] = {region[0], region[1]};

        gegl_clEnqueueWriteImage(gegl_cl_get_command_queue(), in_tex, CL_FALSE,
                                 origin, region, ext->width, 0, &in_data[offset],
                                 0, NULL, NULL);

        gegl_clEnqueueBarrier (gegl_cl_get_command_queue());

        errcode = point_filter_class->cl_process(operation, in_tex, out_tex, global_worksize, result);

        if (errcode > 0) goto error;

        gegl_clEnqueueBarrier (gegl_cl_get_command_queue());

        gegl_clEnqueueReadImage(gegl_cl_get_command_queue(), out_tex, CL_FALSE,
                                origin, region, ext->width, 0, &out_data[offset],
                                0, NULL, NULL);

        gegl_clEnqueueBarrier (gegl_cl_get_command_queue());

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
  if (in_tex)   gegl_clReleaseMemObject (in_tex);
  if (out_tex)  gegl_clReleaseMemObject (out_tex);
  if (in_data)  free (in_data);
  if (out_data) free (out_data);

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
          if (gegl_operation_point_filter_cl_process (operation, input, output, result))
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
