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
#include "gegl/gegl-debug.h"
#include "gegl-types-internal.h"
#include "gegl-operation-point-composer.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include <string.h>

static gboolean gegl_operation_point_composer_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *aux,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

static gboolean
gegl_operation_composer_process2 (GeglOperation        *operation,
                                  GeglOperationContext *context,
                                  const gchar          *output_prop,
                                  const GeglRectangle  *result,
                                  gint                  level);

G_DEFINE_TYPE (GeglOperationPointComposer, gegl_operation_point_composer, GEGL_TYPE_OPERATION_COMPOSER)

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
gegl_operation_point_composer_class_init (GeglOperationPointComposerClass *klass)
{
  /*GObjectClass       *object_class    = G_OBJECT_CLASS (klass);*/
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = gegl_operation_point_composer_process;
  operation_class->prepare = prepare;
  operation_class->no_cache = FALSE;
  operation_class->process = gegl_operation_composer_process2;

  klass->process = NULL;
  klass->cl_process = NULL;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{

}

gboolean gegl_can_do_inplace_processing (GeglOperation       *operation,
                                         GeglBuffer          *input,
                                         const GeglRectangle *result);

/* we replicate the process function from GeglOperationComposer to be
 * able to bail out earlier for some common processing time pitfalls
 */
static gboolean
gegl_operation_composer_process2 (GeglOperation        *operation,
                                  GeglOperationContext *context,
                                  const gchar          *output_prop,
                                  const GeglRectangle  *result,
                                  gint                  level)
{
  GeglOperationComposerClass *klass   = GEGL_OPERATION_COMPOSER_GET_CLASS (operation);
  GeglBuffer                 *input;
  GeglBuffer                 *aux;
  GeglBuffer                 *output;
  gboolean                    success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a composer", output_prop);
      return FALSE;
    }

  input = gegl_operation_context_get_source (context, "input");
  aux   = gegl_operation_context_get_source (context, "aux");

  if (gegl_can_do_inplace_processing (operation, input, result))
    {
      output = g_object_ref (input);
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));
    }
  else
    output = gegl_operation_context_get_target (context, "output");

    {
      gboolean done = FALSE;

      if (result->width == 0 ||
          result->height == 0)
        done = TRUE;

      success = done;
      if (!done)
        {
          success = klass->process (operation, input, aux, output, result, level);

          if (output == GEGL_BUFFER (operation->node->cache))
            gegl_cache_computed (operation->node->cache, result);
        }
      if (input)
         g_object_unref (input);
      if (aux)
         g_object_unref (aux);
    }

  return success;
}

static gboolean
gegl_operation_point_composer_cl_process (GeglOperation       *operation,
                                          GeglBuffer          *input,
                                          GeglBuffer          *aux,
                                          GeglBuffer          *output,
                                          const GeglRectangle *result,
                                          gint                 level)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *aux_format = gegl_operation_get_format (operation, "aux");
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  GeglOperationPointComposerClass *point_composer_class = GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation);

  gint j;
  cl_int cl_err = 0;
  gboolean err;

  /* non-texturizable format! */
  if (!gegl_cl_color_babl (in_format,  NULL) ||
      !gegl_cl_color_babl (aux_format, NULL) ||
      !gegl_cl_color_babl (out_format, NULL))
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Non-texturizable format!");
      return FALSE;
    }

  /* Process */
  {
    GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,   result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                  gint read = gegl_buffer_cl_iterator_add (i, input, result, in_format,  GEGL_CL_BUFFER_READ, GEGL_ABYSS_NONE);
    if (aux)
      {
        gint foo = gegl_buffer_cl_iterator_add (i, aux, result, aux_format,  GEGL_CL_BUFFER_READ, GEGL_ABYSS_NONE);

        while (gegl_buffer_cl_iterator_next (i, &err))
          {
            if (err) return FALSE;
            for (j=0; j < i->n; j++)
              {
                cl_err = point_composer_class->cl_process(operation, i->tex[read][j], i->tex[foo][j], i->tex[0][j],
                                                          i->size[0][j], &i->roi[0][j], level);
                if (cl_err != CL_SUCCESS)
                  {
                    GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s [GeglOperationPointComposer] Kernel",
                               GEGL_OPERATION_CLASS (operation)->name);
                    return FALSE;
                  }
              }
          }
      }
    else
      {
        while (gegl_buffer_cl_iterator_next (i, &err))
          {
            if (err) return FALSE;
            for (j=0; j < i->n; j++)
              {
                cl_err = point_composer_class->cl_process(operation, i->tex[read][j], NULL, i->tex[0][j],
                                                          i->size[0][j], &i->roi[0][j], level);
                if (cl_err != CL_SUCCESS)
                  {
                    GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s [GeglOperationPointComposer] Kernel",
                               GEGL_OPERATION_CLASS (operation)->name);
                    return FALSE;
                  }
              }
          }
      }
  }
  return TRUE;
}

static gboolean
gegl_operation_point_composer_process (GeglOperation       *operation,
                                       GeglBuffer          *input,
                                       GeglBuffer          *aux,
                                       GeglBuffer          *output,
                                       const GeglRectangle *result,
                                       gint                 level)
{
  GeglOperationPointComposerClass *point_composer_class = GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation);
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *aux_format = gegl_operation_get_format (operation, "aux");
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  if ((result->width > 0) && (result->height > 0))
    {
      if (gegl_cl_is_accelerated () && point_composer_class->cl_process)
        {
          if (gegl_operation_point_composer_cl_process (operation, input, aux, output, result, level))
            return TRUE;
        }

      {
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
        gint read = /*output == input ? 0 :*/ gegl_buffer_iterator_add (i, input,  result, level, in_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
        /* using separate read and write iterators for in-place ideally a single
         * readwrite indice would be sufficient
         */

        if (aux)
          {
            gint foo = gegl_buffer_iterator_add (i, aux, result, level, aux_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

            while (gegl_buffer_iterator_next (i))
              {
                 point_composer_class->process (operation, i->data[read], i->data[foo], i->data[0], i->length, &(i->roi[0]), level);
              }
          }
        else
          {
            while (gegl_buffer_iterator_next (i))
              {
                 point_composer_class->process (operation, i->data[read], NULL, i->data[0], i->length, &(i->roi[0]), level);
              }
          }
      }
      return TRUE;
    }
  return TRUE;
}
