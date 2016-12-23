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
#include "gegl-operation-point-composer3.h"
#include "gegl-operation-context.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

typedef struct ThreadData
{
  GeglOperationPointComposer3Class *klass;
  GeglOperation                    *operation;
  guchar                           *input;
  guchar                           *aux;
  guchar                           *aux2;
  guchar                           *output;
  gint                             *pending;
  gint                             *started;
  gint                              level;
  gboolean                          success;
  GeglRectangle                     roi;

  guchar                           *in_tmp;
  guchar                           *aux_tmp;
  guchar                           *aux2_tmp;
  guchar                           *output_tmp;
  const Babl *input_fish;
  const Babl *aux_fish;
  const Babl *aux2_fish;
  const Babl *output_fish;
} ThreadData;

static void thread_process (gpointer thread_data, gpointer unused)
{
  ThreadData *data = thread_data;

  guchar *input = data->input;
  guchar *aux = data->aux;
  guchar *aux2 = data->aux2;
  guchar *output = data->output;
  glong samples = data->roi.width * data->roi.height;

  if (data->input_fish && input)
    {
      babl_process (data->input_fish, data->input, data->in_tmp, samples);
      input = data->in_tmp;
    }
  if (data->aux_fish && aux)
    {
      babl_process (data->aux_fish, data->aux, data->aux_tmp, samples);
      aux = data->aux_tmp;
    }
  if (data->aux2_fish && aux2)
    {
      babl_process (data->aux2_fish, data->aux2, data->aux2_tmp, samples);
      aux2 = data->aux2_tmp;
    }
  if (data->output_fish)
    output = data->output_tmp;

  if (!data->klass->process (data->operation,
                       input, aux, aux2, 
                       output, samples,
                       &data->roi, data->level))
    data->success = FALSE;
  
  if (data->output_fish)
    babl_process (data->output_fish, data->output_tmp, data->output, samples);

  g_atomic_int_add (data->pending, -1);
}

static GThreadPool *thread_pool (void)
{
  static GThreadPool *pool = NULL;
  if (!pool)
    {
      pool =  g_thread_pool_new (thread_process, NULL, gegl_config_threads (),
                                 FALSE, NULL);
    }
  return pool;
}

static gboolean
gegl_operation_composer3_process (GeglOperation        *operation,
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

  if (result->width == 0 || result->height == 0)
  {
    output = gegl_operation_context_get_target (context, "output");
    return TRUE;
  }

  input  = gegl_operation_context_get_source (context, "input");
  output = gegl_operation_context_get_output_maybe_in_place (operation,
                                                             context,
                                                             input,
                                                             result);

  aux   = gegl_operation_context_get_source (context, "aux");
  aux2  = gegl_operation_context_get_source (context, "aux2");

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux != NULL ||
      aux2 != NULL)
    {
      success = klass->process (operation, input, aux, aux2, output, result, level);

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
                 gegl_node_get_operation (operation->node));
    }

  return success;
}

static gboolean gegl_operation_point_composer3_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *aux,
                               GeglBuffer          *aux2,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

G_DEFINE_TYPE (GeglOperationPointComposer3, gegl_operation_point_composer3, GEGL_TYPE_OPERATION_COMPOSER3)

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_babl_rgba_linear_float ();
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "aux2", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
gegl_operation_point_composer3_class_init (GeglOperationPointComposer3Class *klass)
{
  GeglOperationClass          *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposer3Class *composer_class  = GEGL_OPERATION_COMPOSER3_CLASS (klass);

  composer_class->process = gegl_operation_point_composer3_process;
  operation_class->process = gegl_operation_composer3_process;
  operation_class->prepare = prepare;
  operation_class->no_cache =TRUE;
  operation_class->want_in_place = TRUE;
  operation_class->threaded = TRUE;
}

static void
gegl_operation_point_composer3_init (GeglOperationPointComposer3 *self)
{

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
      const Babl *in_buf_format  = input?gegl_buffer_get_format(input):NULL;
      const Babl *aux_buf_format = aux?gegl_buffer_get_format(aux):NULL;
      const Babl *aux2_buf_format = aux2?gegl_buffer_get_format(aux2):NULL;
      const Babl *output_buf_format = output?gegl_buffer_get_format(output):NULL;

      if (gegl_operation_use_threading (operation, result) && result->height > 1)
      {
        gint threads = gegl_config_threads ();
        GThreadPool *pool = thread_pool ();
        ThreadData thread_data[GEGL_MAX_THREADS];
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, output_buf_format, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
        gint foo = 0, bar = 0, read = 0;

        gint in_bpp = input?babl_format_get_bytes_per_pixel (in_format):0;
        gint aux_bpp = aux?babl_format_get_bytes_per_pixel (aux_format):0;
        gint aux2_bpp = aux2?babl_format_get_bytes_per_pixel (aux2_format):0;
        gint out_bpp = babl_format_get_bytes_per_pixel (out_format);

        gint in_buf_bpp = input?babl_format_get_bytes_per_pixel (in_buf_format):0;
        gint aux_buf_bpp = aux?babl_format_get_bytes_per_pixel (aux_buf_format):0;
        gint aux2_buf_bpp = aux2?babl_format_get_bytes_per_pixel (aux2_buf_format):0;
        gint out_buf_bpp = babl_format_get_bytes_per_pixel (output_buf_format);

        gint temp_id = 0;

        if (input)
        {
          if (! babl_format_has_alpha (in_buf_format))
            {
              in_buf_format = in_format;
              in_buf_bpp = in_bpp;
            }

          read = gegl_buffer_iterator_add (i, input, result, level, in_buf_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
          for (gint j = 0; j < threads; j ++)
          {
            if (in_buf_format != in_format)
            {
              thread_data[j].input_fish = babl_fish (in_buf_format, in_format);
              thread_data[j].in_tmp = gegl_temp_buffer (temp_id++, in_bpp * result->width * result->height);
            }
            else
            {
              thread_data[j].input_fish = NULL;
            }
          }
        }
        else
          for (gint j = 0; j < threads; j ++)
            thread_data[j].input_fish = NULL;
        if (aux)
        {
          if (! babl_format_has_alpha (aux_buf_format))
            {
              aux_buf_format = aux_format;
              aux_buf_bpp = aux_bpp;
            }

          foo = gegl_buffer_iterator_add (i, aux, result, level, aux_buf_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
          for (gint j = 0; j < threads; j ++)
          {
            if (aux_buf_format != aux_format)
            {
              thread_data[j].aux_fish = babl_fish (aux_buf_format, aux_format);
              thread_data[j].aux_tmp = gegl_temp_buffer (temp_id++, aux_bpp * result->width * result->height);
            }
            else
            {
              thread_data[j].aux_fish = NULL;
            }
          }
        }
        else
        {
          for (gint j = 0; j < threads; j ++)
            thread_data[j].aux_fish = NULL;
        }
        if (aux2)
        {
          if (! babl_format_has_alpha (aux2_buf_format))
            {
              aux2_buf_format = aux2_format;
              aux2_buf_bpp = aux2_bpp;
            }

          bar = gegl_buffer_iterator_add (i, aux2, result, level, aux2_buf_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
          for (gint j = 0; j < threads; j ++)
          {
            if (aux2_buf_format != aux2_format)
            {
              thread_data[j].aux2_fish = babl_fish (aux2_buf_format, aux2_format);
              thread_data[j].aux2_tmp = gegl_temp_buffer (temp_id++, aux2_bpp * result->width * result->height);
            }
            else
            {
              thread_data[j].aux2_fish = NULL;
            }
          }
        }
        else
        {
          for (gint j = 0; j < threads; j ++)
            thread_data[j].aux2_fish = NULL;
        }

        for (gint j = 0; j < threads; j ++)
        {
          if (output_buf_format != gegl_buffer_get_format (output))
          {
            thread_data[j].output_fish = babl_fish (out_format, output_buf_format);
            thread_data[j].output_tmp = gegl_temp_buffer (temp_id++, out_bpp * result->width * result->height);
          }
          else
          {
            thread_data[j].output_fish = NULL;
          }
        }

        while (gegl_buffer_iterator_next (i))
          {
            gint threads = gegl_config_threads ();
            gint pending;
            gint bit;

            if (i->roi[0].height < threads)
            {
              threads = i->roi[0].height;
            }

            bit = i->roi[0].height / threads;
            pending = threads;

            for (gint j = 0; j < threads; j++)
            {
              thread_data[j].roi.x = (i->roi[0]).x;
              thread_data[j].roi.width = (i->roi[0]).width;
              thread_data[j].roi.y = (i->roi[0]).y + bit * j;
              thread_data[j].roi.height = bit;
            }
            thread_data[threads-1].roi.height = i->roi[0].height - (bit * (threads-1));
            
            for (gint j = 0; j < threads; j++)
            {
              thread_data[j].klass = point_composer3_class;
              thread_data[j].operation = operation;
              thread_data[j].input = input?((guchar*)i->data[read]) + (bit * j * i->roi[0].width * in_buf_bpp):NULL;
              thread_data[j].aux = aux?((guchar*)i->data[foo]) + (bit * j * i->roi[0].width * aux_buf_bpp):NULL;
              thread_data[j].aux2 = aux2?((guchar*)i->data[bar]) + (bit * j * i->roi[0].width * aux2_buf_bpp):NULL;
              thread_data[j].output = ((guchar*)i->data[0]) + (bit * j * i->roi[0].width * out_buf_bpp);
              thread_data[j].pending = &pending;
              thread_data[j].level = level;
              thread_data[j].success = TRUE;
            }
            pending = threads;

            for (gint j = 1; j < threads; j++)
              g_thread_pool_push (pool, &thread_data[j], NULL);
            thread_process (&thread_data[0], NULL);

            while (g_atomic_int_get (&pending)) {};
          }

        return TRUE;
      }
      else
      {
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format,
                                                          GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
        gint foo = 0, bar = 0, read = 0;

        if (input)
          read = gegl_buffer_iterator_add (i, input, result, level, in_format,
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        if (aux)
          foo = gegl_buffer_iterator_add (i, aux, result, level, aux_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        if (aux2)
          bar = gegl_buffer_iterator_add (i, aux2, result, level, aux2_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

        while (gegl_buffer_iterator_next (i))
          {
            point_composer3_class->process (operation, input?i->data[read]:NULL,
                                                       aux?i->data[foo]:NULL,
                                                       aux2?i->data[bar]:NULL,
                                                       i->data[0], i->length, &(i->roi[0]), level);
          }
        return TRUE;
      }
    }
  return TRUE;
}
