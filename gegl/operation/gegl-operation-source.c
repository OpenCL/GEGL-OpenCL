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
#include "gegl-operation-source.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include <string.h>

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LAST
};

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean process      (GeglOperation *operation,
                              GeglNodeContext *context,
                              const gchar   *output_prop,
                              const GeglRectangle *result);
static void     attach       (GeglOperation *operation);



G_DEFINE_TYPE (GeglOperationSource, gegl_operation_source, GEGL_TYPE_OPERATION)

static GeglRectangle get_defined_region    (GeglOperation       *self);
static GeglRectangle compute_input_request (GeglOperation       *operation,
                                            const gchar         *input_pad,
                                            const GeglRectangle *roi);
static GeglRectangle  adjust_result_region (GeglOperation       *operation,
                                            const GeglRectangle *roi);


static void
gegl_operation_source_class_init (GeglOperationSourceClass * klass)
{
  GObjectClass       *gobject_class   = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  operation_class->process = process;
  operation_class->attach  = attach;
  operation_class->adjust_result_region = adjust_result_region;

  operation_class->get_defined_region  = get_defined_region;
  operation_class->compute_input_request = compute_input_request;

  g_object_class_install_property (gobject_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PARAM_PAD_OUTPUT));
}

static void
gegl_operation_source_init (GeglOperationSource *self)
{
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation    = GEGL_OPERATION (self);
  GObjectClass  *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
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
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglOperationSourceClass *klass = GEGL_OPERATION_SOURCE_GET_CLASS (operation);
  GeglBuffer               *output;
  gboolean                  success;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a source operation", output_prop);
      return FALSE;
    }

  g_assert (klass->process);
  output = gegl_node_context_get_target (context, "output");
  success = klass->process (operation, context, output, result);

  return success;
}

static GeglRectangle
get_defined_region (GeglOperation *self)
{
  GeglRectangle result = { 0, 0, 0, 0 };

  g_warning ("Gegl Source '%s' has no proper have_rect function",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
  return result;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  return *roi;
}

static GeglRectangle
adjust_result_region (GeglOperation       *operation,
                      const GeglRectangle *roi)
{
  GeglRectangle result = *roi;
  result = operation->node->have_rect;
  return result;
}
