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
#include <string.h>

#include "gegl.h"
#include "gegl/gegl-debug.h"
#include "gegl-operation-point-filter.h"
#include "gegl-operation-context.h"

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

static gboolean gegl_operation_point_filter_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

static gboolean gegl_operation_point_filter_op_process
                              (GeglOperation       *operation,
                               GeglOperationContext *context,
                               const gchar          *output_pad,
                               const GeglRectangle  *roi,
                               gint                  level);

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
  operation_class->want_in_place = TRUE;

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
                                        const GeglRectangle *result,
                                        gint                 level)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  GeglOperationClass *operation_class = GEGL_OPERATION_GET_CLASS (operation);
  GeglOperationPointFilterClass *point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  GeglBufferClIterator *iter = NULL;

  cl_int cl_err = 0;
  gboolean err;

  /* non-texturizable format! */
  if (!gegl_cl_color_babl (in_format,  NULL) ||
      !gegl_cl_color_babl (out_format, NULL))
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Non-texturizable format!");
      return FALSE;
    }

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "GEGL_OPERATION_POINT_FILTER: %s", operation_class->name);

  /* Process */
  iter = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_CL_BUFFER_WRITE);

  gegl_buffer_cl_iterator_add (iter, input, result, in_format, GEGL_CL_BUFFER_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (iter, &err))
    {
      if (err)
        return FALSE;

      if (point_filter_class->cl_process)
        {
          err = point_filter_class->cl_process (operation, iter->tex[1], iter->tex[0],
                                                iter->size[0], &iter->roi[0], level);
          if (err)
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", operation_class->name);
              gegl_buffer_cl_iterator_stop (iter);
              return FALSE;
            }
        }
      else if (operation_class->cl_data)
        {
          gint p = 0;
          GeglClRunData *cl_data = operation_class->cl_data;

          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], p++, sizeof(cl_mem), (void*)&iter->tex[1]);
          CL_CHECK;
          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], p++, sizeof(cl_mem), (void*)&iter->tex[0]);
          CL_CHECK;

          gegl_operation_cl_set_kernel_args (operation, cl_data->kernel[0], &p, &cl_err);
          CL_CHECK;

          cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                                cl_data->kernel[0], 1,
                                                NULL, &iter->size[0], NULL,
                                                0, NULL, NULL);
          CL_CHECK;
        }
      else
        {
          g_warning ("OpenCL support enabled, but no way to execute");
          gegl_buffer_cl_iterator_stop (iter);
          return FALSE;
        }
    }

  return TRUE;

error:
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", gegl_cl_errstring (cl_err));
  if (iter)
    gegl_buffer_cl_iterator_stop (iter);
  return FALSE;
}

static gboolean
gegl_operation_point_filter_process (GeglOperation       *operation,
                                     GeglBuffer          *input,
                                     GeglBuffer          *output,
                                     const GeglRectangle *result,
                                     gint                 level)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  GeglOperationPointFilterClass *point_filter_class;

  GeglOperationClass *operation_class = GEGL_OPERATION_GET_CLASS (operation);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  if ((result->width > 0) && (result->height > 0))
    {
      if (gegl_operation_use_opencl (operation) &&
          (operation_class->cl_data || point_filter_class->cl_process))
        {
          if (gegl_operation_point_filter_cl_process (operation, input, output, result, level))
            return TRUE;
        }

      {
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
        gint read = /*output == input ? 0 :*/ gegl_buffer_iterator_add (i, input,  result, level, in_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
        /* using separate read and write iterators for in-place ideally a single
         * readwrite indice would be sufficient
         */
          while (gegl_buffer_iterator_next (i))
            point_filter_class->process (operation, i->data[read], i->data[0], i->length, &i->roi[0], level);
      }
    }
  return TRUE;
}

static gboolean gegl_operation_point_filter_op_process
                              (GeglOperation       *operation,
                               GeglOperationContext *context,
                               const gchar          *output_pad,
                               const GeglRectangle  *roi,
                               gint                  level)
{
  GeglOperationClass       *klass = GEGL_OPERATION_GET_CLASS (operation);
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  input = gegl_operation_context_get_source (context, "input");

  if (klass->want_in_place && 
      gegl_can_do_inplace_processing (operation, input, roi))
    {
      output = g_object_ref (input);
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));
    }
  else
    {
      output = gegl_operation_context_get_target (context, "output");
    }

  success = gegl_operation_point_filter_process (operation, input, output, roi, level);

  if (input != NULL)
    g_object_unref (input);
  return success;
}
