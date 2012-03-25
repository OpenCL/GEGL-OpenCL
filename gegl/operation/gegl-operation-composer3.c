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
#include "gegl-types-internal.h"
#include "gegl-operation-composer3.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_AUX,
  PROP_AUX2
};

static void     get_property (GObject              *gobject,
                              guint                 prop_id,
                              GValue               *value,
                              GParamSpec           *pspec);
static void     set_property (GObject              *gobject,
                              guint                 prop_id,
                              const GValue         *value,
                              GParamSpec           *pspec);
static gboolean gegl_operation_composer3_process
                             (GeglOperation        *operation,
                              GeglOperationContext *context,
                              const gchar          *output_prop,
                              const GeglRectangle  *result,
                              gint                  level);
static void     attach       (GeglOperation        *operation);
static GeglNode*detect       (GeglOperation        *operation,
                              gint                  x,
                              gint                  y);

static GeglRectangle get_bounding_box        (GeglOperation        *self);
static GeglRectangle get_required_for_output (GeglOperation        *self,
                                               const gchar         *input_pad,
                                               const GeglRectangle *roi);

G_DEFINE_TYPE (GeglOperationComposer3, gegl_operation_composer3,
               GEGL_TYPE_OPERATION)


static void
gegl_operation_composer3_class_init (GeglOperationComposer3Class * klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->process = gegl_operation_composer3_process;
  operation_class->attach = attach;
  operation_class->detect = detect;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Output pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PARAM_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_AUX,
                                   g_param_spec_object ("aux",
                                                        "Input",
                                                        "Auxiliary image buffer input pad.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_AUX2,
                                   g_param_spec_object ("aux2",
                                                        "Input",
                                                        "Second auxiliary image buffer input pad.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));
}

static void
gegl_operation_composer3_init (GeglOperationComposer3 *self)
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
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "input"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "aux"));

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "aux2"));
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

  input = gegl_operation_context_get_source (context, "input");
  aux   = gegl_operation_context_get_source (context, "aux");
  aux2  = gegl_operation_context_get_source (context, "aux2");
  output = gegl_operation_context_get_target (context, "output");

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux != NULL ||
      aux2 != NULL)
    {
      success = klass->process (operation, input, aux, aux2, output, result, level);

      if (output == GEGL_BUFFER (operation->node->cache))
        gegl_cache_computed (operation->node->cache, result);

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

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result    = { 0, 0, 0, 0 };
  GeglRectangle *in_rect   = gegl_operation_source_get_bounding_box (self, "input");
  GeglRectangle *aux_rect  = gegl_operation_source_get_bounding_box (self, "aux");
  GeglRectangle *aux2_rect = gegl_operation_source_get_bounding_box (self, "aux2");

  if (!in_rect)
    if (!aux_rect)
      if (!aux2_rect)
        return result;
      else
        return *aux2_rect;
    else
      if (!aux2_rect)
        return *aux_rect;
      else
        gegl_rectangle_bounding_box (&result, aux_rect, aux2_rect);
  else
    if (!aux_rect)
      if (!aux2_rect)
        return *in_rect;
      else
        gegl_rectangle_bounding_box (&result, in_rect, aux2_rect);
    else
      if (!aux2_rect)
        gegl_rectangle_bounding_box (&result, in_rect, aux_rect);
      else
        {
          gegl_rectangle_bounding_box (&result, in_rect, aux_rect);
          gegl_rectangle_bounding_box (&result, &result,  aux2_rect);
        }
  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node = gegl_operation_get_source_node (operation, "input");
  GeglNode *aux_node   = gegl_operation_get_source_node (operation, "aux");
  GeglNode *aux2_node  = gegl_operation_get_source_node (operation, "aux2");

  if (input_node)
    input_node = gegl_node_detect (input_node, x, y);
  if (aux_node)
    aux_node = gegl_node_detect (aux_node, x, y);
  if (aux2_node)
    aux2_node = gegl_node_detect (aux2_node, x, y);

  if (aux2_node)
    return aux2_node;
  if (aux_node)
    return aux_node;
  if (input_node)
    return input_node;
  return NULL;
}
