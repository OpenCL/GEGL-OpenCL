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
#include "gegl-operation-sink.h"
#include "graph/gegl-pad.h"
#include <string.h>


enum
{
  PROP_0,
  PROP_INPUT,
  PROP_LAST
};

static void     get_property         (GObject       *gobject,
                                      guint          prop_id,
                                      GValue        *value,
                                      GParamSpec    *pspec);

static void     set_property         (GObject       *gobject,
                                      guint          prop_id,
                                      const GValue  *value,
                                      GParamSpec    *pspec);

static gboolean process              (GeglOperation *operation,
                                      gpointer       context_id,
                                      const gchar   *output_prop);

static void     attach               (GeglOperation *operation);

static GeglRectangle get_defined_region   (GeglOperation *self);

static GeglRectangle compute_input_request (GeglOperation *operation,
                                          const gchar   *input_pad,
                                          GeglRectangle *roi);
G_DEFINE_TYPE (GeglOperationSink, gegl_operation_sink, GEGL_TYPE_OPERATION)


static void
gegl_operation_sink_class_init (GeglOperationSinkClass * klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->process             = process;
  operation_class->attach              = attach;
  operation_class->get_defined_region  = get_defined_region;
  operation_class->compute_input_request = compute_input_request;

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));
}

static void
gegl_operation_sink_init (GeglOperationSink *self)
{
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation    = GEGL_OPERATION (self);
  GObjectClass  *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "input"));
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id,
         const gchar   *output_prop)
{
  GeglOperationSink      *gegl_operation_sink;
  GeglOperationSinkClass *klass;
  GeglBuffer             *input;
  gboolean                success = FALSE;

  gegl_operation_sink = GEGL_OPERATION_SINK (operation);
  klass               = GEGL_OPERATION_SINK_GET_CLASS (operation);

  g_assert (klass->process);

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  if (input) ;
  {
    success = klass->process (operation, context_id);
  }
  return success;
}

static GeglRectangle
get_defined_region (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_defined_region (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle compute_input_request (GeglOperation *operation,
                                          const gchar   *input_pad,
                                          GeglRectangle *roi)
{
  GeglRectangle rect=*roi;
  g_warning ("I'm a sink, why do you call me?, I have no input and that includes the '%s' you asked for.", input_pad);
  return rect;
}
