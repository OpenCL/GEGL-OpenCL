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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */
#include "gegl-operation-filter.h"
#include "graph/gegl-pad.h"
#include <string.h>

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_LAST
};

static void     get_property            (GObject       *gobject,
                                         guint          prop_id,
                                         GValue        *value,
                                         GParamSpec    *pspec);

static void     set_property            (GObject       *gobject,
                                         guint          prop_id,
                                         const GValue  *value,
                                         GParamSpec    *pspec);

static gboolean process                 (GeglOperation *operation,
                                         gpointer       context_id,
                                         const gchar   *output_prop);

static void     attach                  (GeglOperation *operation);
static GeglNode *detect                 (GeglOperation *operation,
                                         gint           x,
                                         gint           y);

static GeglRectangle get_defined_region (GeglOperation *self);
static GeglRectangle compute_input_request (GeglOperation *operation,
                                     const gchar   *input_pad,
                                     GeglRectangle *roi);


G_DEFINE_TYPE (GeglOperationFilter, gegl_operation_filter, GEGL_TYPE_OPERATION)


static void
gegl_operation_filter_class_init (GeglOperationFilterClass * klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->process               = process;
  operation_class->attach                = attach;
  operation_class->detect                = detect;
  operation_class->get_defined_region    = get_defined_region;
  operation_class->compute_input_request = compute_input_request;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));
}

static void
gegl_operation_filter_init (GeglOperationFilter *self)
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
}


static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation, x, y);
  return operation->node;
}


void babl_backtrack (void);

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  switch (prop_id)
    {
      case PROP_OUTPUT:
        g_warning ("shouldn't happen");
        babl_backtrack ();
        break;

      case PROP_INPUT:
        g_warning ("shouldn't happen");
        babl_backtrack ();
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
    {
      case PROP_INPUT:
        g_warning ("shouldn't happen");
        babl_backtrack ();
        break;

      default:
        break;
    }
}


static gboolean
process (GeglOperation *operation,
         gpointer       context_id,
         const gchar   *output_prop)
{
  GeglOperationFilter      *gegl_operation_filter;
  GeglOperationFilterClass *klass;
  GeglBuffer               *input;
  gboolean                  success = FALSE;

  gegl_operation_filter = GEGL_OPERATION_FILTER (operation);
  klass                 = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->process);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a filter", output_prop);
      return FALSE;
    }

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  if (input != NULL)
    {
      success = klass->process (operation, context_id);
    }
  else
    {
      /* if we have the data "graph" associated" we're a proxy-nop, and thus
       * we might legitimatly get NULL (at least layers might)
       */
      if (!g_object_get_data (G_OBJECT (operation->node), "graph"))
        g_warning ("%s received NULL input",
                   gegl_node_get_debug_name (operation->node));
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

static GeglRectangle
compute_input_request (GeglOperation *operation,
                       const gchar   *input_pad,
                       GeglRectangle *roi)
{
  GeglRectangle result = *roi;
  return result;
}
