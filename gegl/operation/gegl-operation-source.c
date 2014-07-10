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
#include "gegl-operation-source.h"
#include "gegl-operation-context.h"
#include "gegl-config.h"

static gboolean gegl_operation_source_process
                             (GeglOperation        *operation,
                              GeglOperationContext *context,
                              const gchar          *output_prop,
                              const GeglRectangle  *result,
                              gint                  level);
static void     attach       (GeglOperation *operation);



G_DEFINE_TYPE (GeglOperationSource, gegl_operation_source, GEGL_TYPE_OPERATION)

static GeglRectangle get_bounding_box          (GeglOperation        *self);
static GeglRectangle get_required_for_output   (GeglOperation        *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *roi);
static GeglRectangle  get_cached_region        (GeglOperation       *operation,
                                                 const GeglRectangle *roi);


static void
gegl_operation_source_class_init (GeglOperationSourceClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process = gegl_operation_source_process;
  operation_class->attach  = attach;
  operation_class->get_cached_region = get_cached_region;

  operation_class->get_bounding_box  = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_source_init (GeglOperationSource *self)
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
}

typedef struct ThreadData
{
  GeglOperationSourceClass *klass;
  GeglOperation            *operation;
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
                       data->output, &data->roi, data->level))
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
gegl_operation_source_process (GeglOperation        *operation,
                               GeglOperationContext *context,
                               const gchar          *output_prop,
                               const GeglRectangle  *result,
                               gint                  level)
{
  GeglOperationSourceClass *klass = GEGL_OPERATION_SOURCE_GET_CLASS (operation);
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a source operation", output_prop);
      return FALSE;
    }

  g_assert (klass->process);
  output = gegl_operation_context_get_target (context, "output");

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
    success = klass->process (operation, output, result, level);
  }

  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle result = { 0, 0, 0, 0 };

  g_warning ("Gegl Source '%s' does not override %s()",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)),
             G_STRFUNC);

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return *roi;
}
