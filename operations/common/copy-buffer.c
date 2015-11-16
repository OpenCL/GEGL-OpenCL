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


#ifdef GEGL_PROPERTIES

property_object (buffer, _("Buffer"), GEGL_TYPE_BUFFER)
    description(_("An already existing GeglBuffer to write incoming buffer data to, or NULL."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE copy-buffer.c

#include "gegl-op.h"

#include "gegl/gegl-debug.h"
#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  if (! format)
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->buffer)
    {
      GeglBuffer *buffer = GEGL_BUFFER (o->buffer);
      const Babl *in_format = gegl_buffer_get_format (input);
      const Babl *out_format = gegl_buffer_get_format (buffer);

      if (gegl_operation_use_opencl (operation)
          && gegl_cl_color_supported (in_format, out_format) == GEGL_CL_COLOR_CONVERT)
        {
          size_t size;
          gboolean err;
          cl_int cl_err = 0;

          GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (buffer,
                                                                 result,
                                                                 out_format,
                                                                 GEGL_CL_BUFFER_WRITE);

          gint read = gegl_buffer_cl_iterator_add (i,
                                                   input,
                                                   result,
                                                   out_format,
                                                   GEGL_CL_BUFFER_READ,
                                                   GEGL_ABYSS_NONE);

          gegl_cl_color_babl (out_format, &size);

          GEGL_NOTE (GEGL_DEBUG_OPENCL,
                     "write-buffer: "
                     "%p %p %s %s {%d %d %d %d}",
                     input,
                     buffer,
                     babl_get_name (in_format),
                     babl_get_name (out_format),
                     result->x,
                     result->y,
                     result->width,
                     result->height);

          while (gegl_buffer_cl_iterator_next (i, &err))
            {
              if (err) break;

              cl_err = gegl_clEnqueueCopyBuffer (gegl_cl_get_command_queue (),
                                                 i->tex[read],
                                                 i->tex[0],
                                                 0,
                                                 0,
                                                 i->size[0] * size,
                                                 0,
                                                 NULL,
                                                 NULL);

              if (cl_err != CL_SUCCESS)
                {
                  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", gegl_cl_errstring (cl_err));
                  break;
                }
            }

          if (cl_err || err)
            gegl_buffer_copy (input, result, GEGL_ABYSS_NONE,
                              buffer, result);
        }
      else
        gegl_buffer_copy (input, result, GEGL_ABYSS_NONE,
                          buffer, result);
    }

  if (output)
    gegl_buffer_copy (input, result, GEGL_ABYSS_NONE,
                      output, result);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare  = prepare;
  operation_class->no_cache = TRUE;
  filter_class->process     = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:copy-buffer",
    "title",       _("Copy Buffer"),
    "categories" , "programming",
    "description", _("Writes image data to an already existing buffer"),
    NULL);
}

#endif
