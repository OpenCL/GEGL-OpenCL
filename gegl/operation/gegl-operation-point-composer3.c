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
#include "gegl-operation-point-composer3.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include <string.h>

static gboolean gegl_operation_point_composer3_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *aux,
                               GeglBuffer          *aux2,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

static gboolean
gegl_operation_composer3_process2 (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level);

G_DEFINE_TYPE (GeglOperationPointComposer3, gegl_operation_point_composer3, GEGL_TYPE_OPERATION_COMPOSER3)

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "aux2", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
gegl_operation_point_composer3_class_init (GeglOperationPointComposer3Class *klass)
{
  /*GObjectClass       *object_class    = G_OBJECT_CLASS (klass);*/
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposer3Class *composer_class = GEGL_OPERATION_COMPOSER3_CLASS (klass);

  composer_class->process = gegl_operation_point_composer3_process;
  operation_class->prepare = prepare;
  operation_class->no_cache =TRUE;
  operation_class->process = gegl_operation_composer3_process2;
}

static void
gegl_operation_point_composer3_init (GeglOperationPointComposer3 *self)
{

}

/* we replicate the process function from GeglOperationComposer3 to be
 * able to bail out earlier for some common processing time pitfalls
 */
static gboolean
gegl_operation_composer3_process2 (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GeglOperationComposer3Class *klass   = GEGL_OPERATION_COMPOSER3_GET_CLASS (operation);
  GeglBuffer                  *input;
  GeglBuffer                  *aux;
  GeglBuffer                  *aux2;
  GeglBuffer                  *output;
  gboolean                     success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a composer", output_prop);
      return FALSE;
    }

  input = gegl_operation_context_get_source (context, "input");
  aux   = gegl_operation_context_get_source (context, "aux");
  aux2  = gegl_operation_context_get_source (context, "aux2");

  /* we could be even faster by not alway writing to this buffer, that
   * would potentially break other assumptions we want to make from the
   * GEGL core so we avoid doing that
   */
  output = gegl_operation_context_get_target (context, "output");


  if (input != NULL ||
      aux != NULL ||
      aux2 != NULL)
    {
      gboolean done = FALSE;

      if (result->width == 0 ||
          result->height == 0)
        done = TRUE;

      success = done;
      if (!done)
        {
          success = klass->process (operation, input, aux, aux2, output, result, context->level);

          if (output == GEGL_BUFFER (operation->node->cache))
            gegl_cache_computed (operation->node->cache, result);
        }
      if (input)
         g_object_unref (input);
      if (aux)
         g_object_unref (aux);
      if (aux2)
         g_object_unref (aux2);
    }
  else
    {
      g_warning ("%s received NULL input, aux, and aux2",
                 gegl_node_get_debug_name (operation->node));
    }

  return success;
}

static gboolean
gegl_operation_point_composer3_process (GeglOperation       *operation,
                                        GeglBuffer          *input,
                                        GeglBuffer          *aux,
                                        GeglBuffer          *aux2,
                                        GeglBuffer          *output,
                                        const GeglRectangle *result,
                                        gint                 level)
{
  GeglOperationPointComposer3Class *point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_GET_CLASS (operation);
  const Babl *in_format   = gegl_operation_get_format (operation, "input");
  const Babl *aux_format  = gegl_operation_get_format (operation, "aux");
  const Babl *aux2_format = gegl_operation_get_format (operation, "aux2");
  const Babl *out_format  = gegl_operation_get_format (operation, "output");

  if ((result->width > 0) && (result->height > 0))
    {
      GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
      gint read  = gegl_buffer_iterator_add (i, input, result, level, in_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

      if (aux)
        {
          gint foo = gegl_buffer_iterator_add (i, aux, result, level, aux_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
          if (aux2)
            {
              gint bar = gegl_buffer_iterator_add (i, aux2, result, level, aux2_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

              while (gegl_buffer_iterator_next (i))
                {
                   point_composer3_class->process (operation, i->data[read], i->data[foo], i->data[bar], i->data[0], i->length, &(i->roi[0]), level);
                }
            }
          else
            {
              while (gegl_buffer_iterator_next (i))
                {
                   point_composer3_class->process (operation, i->data[read], i->data[foo], NULL, i->data[0], i->length, &(i->roi[0]), level);
                }
            }
        }
      else
        {
          if (aux2)
            {
              gint bar = gegl_buffer_iterator_add (i, aux2, result, level, aux2_format, GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
              while (gegl_buffer_iterator_next (i))
                {
                   point_composer3_class->process (operation, i->data[read], NULL, i->data[bar], i->data[0], i->length, &(i->roi[0]), level);
                }
            }
          else
            {
              while (gegl_buffer_iterator_next (i))
                {
                   point_composer3_class->process (operation, i->data[read], NULL, NULL, i->data[0], i->length, &(i->roi[0]), level);
                }
            }
        }
      return TRUE;
    }
  return TRUE;
}
