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
 * Copyright 2006, 2014 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-filter.h"
#include "gegl-operation-context.h"
#include "gegl-config.h"

static gboolean gegl_operation_filter_process
                                      (GeglOperation        *operation,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level);

static void     attach                 (GeglOperation *operation);
static GeglNode *detect                (GeglOperation *operation,
                                        gint           x,
                                        gint           y);

static GeglRectangle get_bounding_box          (GeglOperation       *self);
static GeglRectangle get_required_for_output   (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *roi);


G_DEFINE_TYPE (GeglOperationFilter, gegl_operation_filter, GEGL_TYPE_OPERATION)


static void
gegl_operation_filter_class_init (GeglOperationFilterClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process                 = gegl_operation_filter_process;
  operation_class->threaded                = TRUE;
  operation_class->attach                  = attach;
  operation_class->detect                  = detect;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_filter_init (GeglOperationFilter *self)
{
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READABLE |
                               GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}


static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_node_detect (input_node, x, y);
  return operation->node;
}

typedef struct ThreadData
{
  GeglOperationFilterClass *klass;
  GeglOperation            *operation;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gint                     *pending;
  gint                      level;
  gboolean                  success;
  GeglRectangle             roi;
} ThreadData;

static void thread_process (gpointer thread_data, gpointer unused)
{
  ThreadData *data = thread_data;
  if (!data->klass->process (data->operation,
                       data->input, data->output, &data->roi, data->level))
    data->success = FALSE;
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
gegl_operation_filter_process (GeglOperation        *operation,
                               GeglOperationContext *context,
                               const gchar          *output_prop,
                               const GeglRectangle  *result,
                               gint                  level)
{
  GeglOperationFilterClass *klass;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  klass = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->process);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a filter", output_prop);
      return FALSE;
    }

  input  = gegl_operation_context_get_source (context, "input");
  output = gegl_operation_context_get_output_maybe_in_place (operation,
                                                             context,
                                                             input,
                                                             result);

  if (gegl_operation_use_threading (operation, result))
  {
    gint threads = gegl_config_threads ();
    GThreadPool *pool = thread_pool ();
    ThreadData thread_data[GEGL_MAX_THREADS];
    gint pending = threads;

    if (result->width > result->height)
    {
      gint bit = result->width / threads;
      for (gint j = 0; j < threads; j++)
      {
        thread_data[j].roi.y = result->y;
        thread_data[j].roi.height = result->height;
        thread_data[j].roi.x = result->x + bit * j;
        thread_data[j].roi.width = bit;
      }
      thread_data[threads-1].roi.width = result->width - (bit * (threads-1));
    }
    else
    {
      gint bit = result->height / threads;
      for (gint j = 0; j < threads; j++)
      {
        thread_data[j].roi.x = result->x;
        thread_data[j].roi.width = result->width;
        thread_data[j].roi.y = result->y + bit * j;
        thread_data[j].roi.height = bit;
      }
      thread_data[threads-1].roi.height = result->height - (bit * (threads-1));
    }
    for (gint i = 0; i < threads; i++)
    {
      thread_data[i].klass = klass;
      thread_data[i].operation = operation;
      thread_data[i].input = input;
      thread_data[i].output = output;
      thread_data[i].pending = &pending;
      thread_data[i].level = level;
      thread_data[i].success = TRUE;
    }

    for (gint i = 1; i < threads; i++)
      g_thread_pool_push (pool, &thread_data[i], NULL);
    thread_process (&thread_data[0], NULL);

    while (g_atomic_int_get (&pending)) {};

    success = thread_data[0].success;
  }
  else
  {
    success = klass->process (operation, input, output, result, level);
  }

  g_clear_object (&input);
  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}
