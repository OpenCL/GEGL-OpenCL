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


#define GEGL_INTERNAL
#include "config.h"

#include <glib-object.h>
#include "gegl-types.h"
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
                               const GeglRectangle *result);

G_DEFINE_TYPE (GeglOperationPointComposer, gegl_operation_point_composer, GEGL_TYPE_OPERATION_COMPOSER)

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");
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
  operation_class->no_cache =TRUE;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{

}

#if 0 /* FIXME: this should be re-enabled, possibly by skipping the point-composer class duplicating that
       * code and directly implement on top of GeglOperation
       */
static gboolean
fast_paths (GeglOperation       *operation,
            GeglNodeContext     *context,
            const Babl          *in_format,
            const Babl          *aux_format,
            const Babl          *out_format,
            const GeglRectangle *result);
#endif

static gboolean
gegl_operation_point_composer_process (GeglOperation       *operation,
                                       GeglBuffer          *input,
                                       GeglBuffer          *aux,
                                       GeglBuffer          *output,
                                       const GeglRectangle *result)
{
  GeglPad    *pad;
  const Babl *in_format;
  const Babl *aux_format;
  const Babl *out_format;

  pad       = gegl_node_get_pad (operation->node, "input");
  in_format = gegl_pad_get_format (pad);
  if (!in_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (in_format);

  pad        = gegl_node_get_pad (operation->node, "aux");
  aux_format = gegl_pad_get_format (pad);
  if (!aux_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (aux_format);

  pad        = gegl_node_get_pad (operation->node, "output");
  out_format = gegl_pad_get_format (pad);
  if (!out_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (out_format);

  /* XXX: when disabling fast_paths everything should work, albeit slower,
   * disabling of fast paths can be tried when things appear strange, debugging
   * with and without fast paths when tracking buffer leaks is probably also a
   * good idea. NB! some of the OpenRaster meta ops, depends on the
   * short-circuiting happening in fast_paths.
   * */
#if 0
  if (0 && fast_paths (operation, context,
                       in_format,
                       aux_format,
                       out_format,
                       result))
    return TRUE;
#endif

#if 0
  /* retrieve the buffer we're writing to from GEGL */
  output = gegl_node_context_get_target (context, "output");
#endif

  if ((result->width > 0) && (result->height > 0))
    {
      gfloat *in_buf = NULL, *out_buf = NULL, *aux_buf = NULL;

      in_buf = gegl_malloc (in_format->format.bytes_per_pixel *
                         output->extent.width * output->extent.height);
      if (in_format == out_format)
        {
          out_buf = in_buf;
        }
      else
        {
          out_buf = gegl_malloc (out_format->format.bytes_per_pixel *
                              output->extent.width * output->extent.height);
        }

      gegl_buffer_get (input, 1.0, result, in_format, in_buf, GEGL_AUTO_ROWSTRIDE);

      if (aux)
        {
          aux_buf = gegl_malloc (aux_format->format.bytes_per_pixel *
                             output->extent.width * output->extent.height);
          gegl_buffer_get (aux, 1.0, result, aux_format, aux_buf, GEGL_AUTO_ROWSTRIDE);
        }
      {
        GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
          operation,
          in_buf,
          aux_buf,
          out_buf,
          output->extent.width * output->extent.height);
      }

      gegl_buffer_set (output, NULL, out_format, out_buf, GEGL_AUTO_ROWSTRIDE);

      gegl_free (in_buf);
      if (in_format != out_format)
        gegl_free (out_buf);
      if (aux)
        gegl_free (aux_buf);
    }
  return TRUE;
}

#if 0
static gboolean
fast_paths (GeglOperation       *operation,
            GeglNodeContext     *context,
            const Babl          *in_format,
            const Babl          *aux_format,
            const Babl          *out_format,
            const GeglRectangle *result)
{
  GeglBuffer  *input = gegl_node_context_get_source (context, "input");
  GeglBuffer  *aux   = gegl_node_context_get_source (context, "aux");

  if (!input && aux)
    {
      g_object_ref (aux);
      gegl_node_context_set_object (context, "output", G_OBJECT (aux));
      return TRUE;
    }

  {
    if ((result->width > 0) && (result->height > 0))
      {

        const gchar *op = gegl_node_get_operation (operation->node);
        if (!strcmp (op, "over")) /* these optimizations probably apply to more than
                                     over */
          {
/* SKIP_EMPTY_IN */
            {
              const GeglRectangle *in_abyss;

              in_abyss = gegl_buffer_get_abyss (input);

              if ((!input ||
                   !gegl_rectangle_intersect (NULL, in_abyss, result)) &&
                  aux)
                {
                  const GeglRectangle *aux_abyss;
                  aux_abyss = gegl_buffer_get_abyss (aux);

                  if (!gegl_rectangle_intersect (NULL, aux_abyss, result))
                    {
                      GeglBuffer *output = gegl_buffer_new (NULL, NULL);
                      gegl_node_context_set_object (context, "output", G_OBJECT (output));
                      return TRUE;
                    }
                  g_object_ref (aux);
                  gegl_node_context_set_object (context, "output", G_OBJECT (aux));
                  return TRUE;
                }
            }
/* SKIP_EMPTY_AUX */
            {
              const GeglRectangle *aux_abyss = NULL;

              if (aux)
                aux_abyss = gegl_buffer_get_abyss (aux);

              if (!aux ||
                  (aux && !gegl_rectangle_intersect (NULL, aux_abyss, result)))
                {
                  g_object_ref (input);
                  gegl_node_context_set_object (context, "output", G_OBJECT (input));
                  return TRUE;
                }
            }
          }
      }
    else
      {
        GeglBuffer *output = gegl_buffer_new (NULL, out_format);
        gegl_node_context_set_object (context, "output", G_OBJECT (output));
        return TRUE;
      }
  }
  return FALSE;
}
#endif
