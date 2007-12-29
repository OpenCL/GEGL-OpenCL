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
#include "gegl-operation-point-composer.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include <string.h>

static gboolean process_inner (GeglOperation *operation,
                               gpointer       context_id);

G_DEFINE_TYPE (GeglOperationPointComposer, gegl_operation_point_composer, GEGL_TYPE_OPERATION_COMPOSER)

static void prepare (GeglOperation *operation,
                     gpointer       context_id)
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

  composer_class->process = process_inner;
  operation_class->prepare = prepare;
  operation_class->no_cache =TRUE;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{

}

static gboolean
fast_paths (GeglOperation *operation,
            gpointer       context_id,
            Babl          *in_format,
            Babl          *aux_format,
            Babl          *out_format);

static gboolean
process_inner (GeglOperation *operation,
               gpointer       context_id)
{
  GeglBuffer          *input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  GeglBuffer          *aux   = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "aux"));
  const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
  GeglBuffer          *output;
  GeglPad             *pad;
  Babl                *in_format;
  Babl                *aux_format;
  Babl                *out_format;

  pad       = gegl_node_get_pad (operation->node, "input");
  in_format = pad->format;
  if (!in_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (in_format);

  pad       = gegl_node_get_pad (operation->node, "aux");
  aux_format = pad->format;
  if (!aux_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (aux_format);

  pad        = gegl_node_get_pad (operation->node, "output");
  out_format = pad->format;
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
  if (fast_paths (operation, context_id, in_format, aux_format, out_format))
    return TRUE;

  /* retrieve the buffer we're writing to from GEGL */
  output = gegl_operation_get_target (operation, context_id, "output");

  if ((result->width > 0) && (result->height > 0))
    {
      gfloat *in_buf = NULL, *out_buf = NULL, *aux_buf = NULL;

      in_buf = g_malloc (in_format->format.bytes_per_pixel *
                         output->extent.width * output->extent.height);
      if (in_format == out_format)
        {
          out_buf = in_buf;
        }
      else
        {
          out_buf = g_malloc (out_format->format.bytes_per_pixel *
                              output->extent.width * output->extent.height);
        }

      gegl_buffer_get (input, 1.0, result, in_format, in_buf, GEGL_AUTO_ROWSTRIDE);

      if (aux)
        {
          aux_buf = g_malloc (aux_format->format.bytes_per_pixel *
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

      gegl_buffer_set (output, NULL, out_format, out_buf);

      g_free (in_buf);
      if (in_format != out_format)
        g_free (out_buf);
      if (aux)
        g_free (aux_buf);
    }
  return TRUE;
}


static gboolean
fast_paths (GeglOperation *operation,
            gpointer       context_id,
            Babl          *in_format,
            Babl          *aux_format,
            Babl          *out_format)
{
  GeglBuffer          *input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  GeglBuffer          *aux   = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "aux"));
  const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);

  if (!input && aux)
    {
      g_object_ref (aux);
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (aux));
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
                      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
                      return TRUE;
                    }
                  g_object_ref (aux);
                  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (aux));
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
                  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (input));
                  return TRUE;
                }
            }
          }
      }
    else
      {
        GeglBuffer *output = gegl_buffer_new (NULL, out_format);
        gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
        return TRUE;
      }
  }
  return FALSE;
}
