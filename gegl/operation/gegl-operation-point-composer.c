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

static gboolean
gegl_operation_composer_process2 (GeglOperation       *operation,
                        GeglNodeContext     *context,
                        const gchar         *output_prop,
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
  operation_class->process = gegl_operation_composer_process2;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{

}

/* we replicate the process function from GeglOperationComposer to be
 * able to bail out earlier for some common processing time pitfalls
 */
static gboolean
gegl_operation_composer_process2 (GeglOperation       *operation,
                                  GeglNodeContext     *context,
                                  const gchar         *output_prop,
                                  const GeglRectangle *result)
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

  input = gegl_node_context_get_source (context, "input");
  aux   = gegl_node_context_get_source (context, "aux");

  /* we could be even faster by not alway writing to this buffer, that
   * would potentially break other assumptions we want to make from the
   * GEGL core so we avoid doing that
   */
  output = gegl_node_context_get_target (context, "output");


  if (input != NULL ||
      aux != NULL)
    {
      gboolean done = FALSE;

      if (result->width == 0 ||
          result->height == 0)
        done = TRUE;

#if 1  /* this can be set to 0, and everything should work normally,
          but some fast paths would not be taken */
      if (!strcmp (gegl_node_get_operation (operation->node), "over"))
        {
          /* these optimizations probably apply to more than over */

          if ((result->width > 0) && (result->height > 0))

          if (input && aux==NULL)
            {
              gegl_buffer_copy (input, result, output, result);
              done = TRUE;
            }

          /* SKIP_EMPTY_IN */
          if(!done)
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
                      done = TRUE;
                    }
                  else
                    {
                      gegl_buffer_copy (aux, result, output, result);
                      done = TRUE;
                    }
                }
            }
/* SKIP_EMPTY_AUX */
            if(!done)
              {
              const GeglRectangle *aux_abyss = NULL;

              if (aux)
                aux_abyss = gegl_buffer_get_abyss (aux);

              if (!aux ||
                  (aux && !gegl_rectangle_intersect (NULL, aux_abyss, result)))
                {
                  gegl_buffer_copy (input, result, output, result);
                  done = TRUE;
                }
            }
      }
#endif

      success = done;
      if (!done)
        {
          success = klass->process (operation, input, aux, output, result);
        }
      if (input)
         g_object_unref (input);
      if (aux)
         g_object_unref (aux);
    }
  else
    {
      g_warning ("%s received NULL input and aux",
                 gegl_node_get_debug_name (operation->node));
    }

  return success;
}

static gboolean
gegl_operation_point_composer_process (GeglOperation       *operation,
                                       GeglBuffer          *input,
                                       GeglBuffer          *aux,
                                       GeglBuffer          *output,
                                       const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *aux_format = gegl_operation_get_format (operation, "aux");
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  if ((result->width > 0) && (result->height > 0))
    {
      gfloat *in_buf = NULL, *out_buf = NULL, *aux_buf = NULL;

      if (gegl_buffer_scan_compatible (input, result->x, result->y,
                                       output, result->x, result->y))
        {
          gint input_bpp  = in_format->format.bytes_per_pixel;
          gint output_bpp = output->format->format.bytes_per_pixel;
          gint aux_bpp    = aux?aux->format->format.bytes_per_pixel:1;

          Babl *infish;
          Babl *outfish;
          GeglBufferScanIterator read;
          GeglBufferScanIterator write;
          gboolean a = FALSE, b = FALSE;

          gegl_buffer_scan_iterator_init (&read,  input,  *result, FALSE);
          gegl_buffer_scan_iterator_init (&write, output, *result, TRUE);
          
          infish = babl_fish (input->format, in_format);
          outfish = babl_fish (out_format, output->format);

	 /* FIXME use direct access when possible (avoid conversions and buffers) 
          */
           {
              GeglRectangle roi;
              if (aux)
                aux_buf = gegl_malloc (aux_bpp * write.max_size);

              if (input->format == in_format &&
                  output->format == out_format)
                {
                  while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                           (b = gegl_buffer_scan_iterator_next (&write)))
                    {
                   gegl_buffer_scan_iterator_get_rectangle (&write, &roi);

                   g_assert (read.length == write.length);

                   if (aux) gegl_buffer_get (aux, 1.0, &roi, aux_format, aux_buf,
                                             GEGL_AUTO_ROWSTRIDE);

                   GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
                            operation,
                            read.data,
                            aux_buf, /* can be NULL */
                            write.data,
                            write.length);
                    }
                }
              else if (input->format == in_format &&
                       output->format != out_format)
                {
                  out_buf = gegl_malloc (output_bpp * write.max_size);
                  while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                           (b = gegl_buffer_scan_iterator_next (&write)))
                    {
                      gegl_buffer_scan_iterator_get_rectangle (&write, &roi);

                      g_assert (read.length == write.length);

                      if (aux) gegl_buffer_get (aux, 1.0, &roi, aux_format, aux_buf,
                                                GEGL_AUTO_ROWSTRIDE);

                      GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
                            operation,
                            read.data,
                            aux_buf, /* can be NULL */
                            out_buf,
                            write.length);
                      babl_process (outfish, out_buf, write.data, write.length);
                    }
                  gegl_free (out_buf);
                }
              else if (input->format != in_format &&
                       output->format == out_format)
                {
                  in_buf  = gegl_malloc (input_bpp * read.max_size);
                  while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                           (b = gegl_buffer_scan_iterator_next (&write)))
                    {
                      gegl_buffer_scan_iterator_get_rectangle (&write, &roi);

                      g_assert (read.length == write.length);
                      babl_process (infish, read.data, in_buf, read.length);

                      if (aux) gegl_buffer_get (aux, 1.0, &roi, aux_format, aux_buf,
                                                GEGL_AUTO_ROWSTRIDE);

                      GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
                            operation,
                            in_buf,
                            aux_buf, /* can be NULL */
                            write.data,
                            write.length);
                    }
                  gegl_free (in_buf);
                }
              else if (input->format != in_format &&
                       output->format != out_format)
                {
                  in_buf  = gegl_malloc (input_bpp * read.max_size);
                  out_buf = gegl_malloc (output_bpp * write.max_size);
                  while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                           (b = gegl_buffer_scan_iterator_next (&write)))
                    {
                      gegl_buffer_scan_iterator_get_rectangle (&write, &roi);

                      g_assert (read.length == write.length);
                      babl_process (infish, read.data, in_buf, read.length);

                      if (aux) gegl_buffer_get (aux, 1.0, &roi, aux_format, aux_buf,
                                                GEGL_AUTO_ROWSTRIDE);

                      GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
                            operation,
                            in_buf,
                            aux_buf, /* can be NULL */
                            out_buf,
                            write.length);
                      babl_process (outfish, out_buf, write.data, write.length);
                    }
                  gegl_free (in_buf);
                  gegl_free (out_buf);
                }

            }

          if (aux)
            gegl_free (aux_buf);
          return TRUE;
        }

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
