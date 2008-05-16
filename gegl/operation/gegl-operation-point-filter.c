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
#include "gegl-operation-point-filter.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-node.h"
#include "gegl-utils.h"
#include <string.h>

#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"

static gboolean process_inner (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *output,
                               const GeglRectangle *result);

G_DEFINE_TYPE (GeglOperationPointFilter, gegl_operation_point_filter, GEGL_TYPE_OPERATION_FILTER)

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
gegl_operation_point_filter_class_init (GeglOperationPointFilterClass *klass)
{
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  filter_class->process = process_inner;
  operation_class->prepare = prepare;
}

static void
gegl_operation_point_filter_init (GeglOperationPointFilter *self)
{
}


static gboolean
process_inner (GeglOperation       *operation,
               GeglBuffer          *input,
               GeglBuffer          *output,
               const GeglRectangle *result)
{
  GeglPad    *pad;
  const Babl *in_format;
  const Babl *out_format;
  GeglOperationPointFilterClass *point_filter_class;

  point_filter_class = GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation);

  pad       = gegl_node_get_pad (operation->node, "input");
  in_format = gegl_pad_get_format (pad);
  if (!in_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (in_format);

  pad        = gegl_node_get_pad (operation->node, "output");
  out_format = gegl_pad_get_format (pad);
  if (!out_format)
    {
      g_warning ("%s", gegl_node_get_debug_name (operation->node));
    }
  g_assert (out_format);

  if ((result->width > 0) && (result->height > 0))
    {
      if (gegl_buffer_scan_compatible (input, output))
        /* We can use the fastest possible path with the least possible
         * copies using paralell scan iteratator with possibly direct
         * read write access to buffers.
         */
        {
          GTimer       *timer       = g_timer_new ();
          gint input_bpp  = in_format->format.bytes_per_pixel;
          gint output_bpp = output->format->format.bytes_per_pixel;
          gpointer     *in_buf = NULL;
          gpointer     *out_buf = NULL;
          Babl         *infish;
          Babl         *outfish;

          GeglBufferScanIterator read;
          GeglBufferScanIterator write;
          gegl_buffer_scan_iterator_init (&read,  input,  *result, FALSE);
          gegl_buffer_scan_iterator_init (&write, output, *result, TRUE);

          g_assert (input->tile_storage->tile_width == output->tile_storage->tile_width);

          in_buf  = gegl_malloc (input_bpp * input->tile_storage->tile_width *
                                             input->tile_storage->tile_height);
          out_buf = gegl_malloc (output_bpp * output->tile_storage->tile_width *
                                              output->tile_storage->tile_height);

          infish = babl_fish (input->format, in_format);
          outfish = babl_fish (out_format, output->format);
         
          gegl_buffer_lock (output); 
          {
          gboolean a = FALSE, b = FALSE;
          if (in_format == input->format &&
              out_format == output->format)
            {
              while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                       (b = gegl_buffer_scan_iterator_next (&write)))
                {
                  g_assert (read.width == write.width);
                  point_filter_class->process (operation, read.data, write.data, write.width);
                }
            }
          else if (in_format == input->format &&
                   out_format != output->format)
            {
              while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                       (b = gegl_buffer_scan_iterator_next (&write)))
                {
                  g_assert (read.width == write.width);
                  point_filter_class->process (operation, read.data, out_buf, read.width);
                  babl_process (outfish, out_buf, write.data, write.width);
                }
            }
          else if (in_format != input->format &&
                   out_format == output->format)
            {
              while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                       (b = gegl_buffer_scan_iterator_next (&write)))
                {
                  if (read.width < 0)
                    continue;
                  g_assert (read.width == write.width);
                  babl_process (infish, read.data, in_buf, read.width);
                  point_filter_class->process (operation, in_buf, write.data, read.width);
                }
            }
          else if (in_format != input->format &&
                   out_format != output->format)
            {
              while (  (a = gegl_buffer_scan_iterator_next (&read)) &&
                       (b = gegl_buffer_scan_iterator_next (&write)))
                {
                  g_assert (read.width == write.width);
                  babl_process (infish, read.data, in_buf, read.width);
                  point_filter_class->process (operation, in_buf, out_buf, read.width);
                  babl_process (outfish, out_buf, write.data, write.width);
                }
            }

          if (a)
            while (gegl_buffer_scan_iterator_next (&read));
          if (b)
            while (gegl_buffer_scan_iterator_next (&write));
          }

          gegl_free (in_buf);
          gegl_free (out_buf);
          gegl_buffer_unlock (output); 

          g_printerr ("%s: %d x %d %g Mpixels/sec\n",
              G_STRFUNC,
              output->extent.width,
              output->extent.height,
              ( output->extent.width* output->extent.height) /
               (1000000 * g_timer_elapsed (timer, NULL)));
          g_timer_destroy (timer);
        }
    /* eek: this fails,..
      if (!(input->width == output->width &&
          input->height == output->height))
        {
          g_warning ("%ix%i -> %ix%i", input->width, input->height, output->width, output->height);
        }
      g_assert (input->width == output->width &&
                input->height == output->height);
     */

#if 0
      else
        {
          gfloat *in_buf;
          gfloat *out_buf;
          in_buf = gegl_malloc (in_format->format.bytes_per_pixel *
                             input->extent.width * input->extent.height);
          out_buf = gegl_malloc (out_format->format.bytes_per_pixel *
                             output->extent.width * output->extent.height);

          gegl_buffer_get (input, 1.0, result, in_format, in_buf, GEGL_AUTO_ROWSTRIDE);

          GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->process (
            operation,
            in_buf,
            out_buf,
            output->extent.width * output->extent.height);

          gegl_buffer_set (output, result, out_format, out_buf, GEGL_AUTO_ROWSTRIDE);
          gegl_free (in_buf);
          gegl_free (out_buf);
#else
      else
        {
          gfloat *in_buf;
          GeglRectangle roi;
          gint skip = 32;
          gfloat *out_buf;
          in_buf = gegl_malloc (in_format->format.bytes_per_pixel * result->width * skip);
          out_buf = gegl_malloc (out_format->format.bytes_per_pixel * result->width * skip);


          roi = *result;
          while (roi.y < result->y + result->height && skip >0 )
            {
              for (roi.height=skip; (roi.y + skip <= result->y + result->height);
                   roi.y+=skip)
                {
                  gegl_buffer_get (input, 1.0, &roi, in_format, in_buf, GEGL_AUTO_ROWSTRIDE);

                  GEGL_OPERATION_POINT_FILTER_GET_CLASS (operation)->process (
                    operation,
                    in_buf,
                    out_buf,
                    result->width * skip);

                  gegl_buffer_set (output, &roi, out_format, out_buf, GEGL_AUTO_ROWSTRIDE);
                }
                skip /= 2;
              }


          gegl_free (in_buf);
          gegl_free (out_buf);
        }
#endif
    }
  return TRUE;
}
