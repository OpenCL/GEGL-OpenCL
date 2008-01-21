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

#ifndef __GEGL_OPERATION_NOP_H__
#define __GEGL_OPERATION_NOP_H__

#include <gegl-plugin.h>

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_NOP            (gegl_operation_nop_get_type ())
#define GEGL_OPERATION_NOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_NOP, GeglOperationNop))
#define GEGL_OPERATION_NOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_NOP, GeglOperationNopClass))
#define GEGL_IS_OPERATION_NOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_NOP))
#define GEGL_IS_OPERATION_NOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_NOP))
#define GEGL_OPERATION_NOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_NOP, GeglOperationNopClass))

typedef struct _GeglOperationNop      GeglOperationNop;
typedef struct _GeglOperationNopClass GeglOperationNopClass;

struct _GeglOperationNop
{
  GeglOperation parent_instance;
};

struct _GeglOperationNopClass
{
  GeglOperationClass parent_class;
};

GType gegl_operation_nop_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEGL_OPERATION_NOP_H__ */

/***************************************************************************/

#include "graph/gegl-pad.h"
#include "graph/gegl-node.h"
#include <math.h>
#include <string.h>


enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT
};

static gboolean        process                 (GeglOperation       *operation,
                                                GeglNodeContext     *context,
                                                const gchar         *output_prop,
                                                const GeglRectangle *result);
static void            attach                  (GeglOperation       *operation);
static GeglNode      * detect                  (GeglOperation       *operation,
                                                gint                 x,
                                                gint                 y);
static GeglRectangle   get_defined_region      (GeglOperation       *operation);
static GeglRectangle   compute_input_request   (GeglOperation       *operation,
                                                const gchar         *input_pad,
                                                const GeglRectangle *roi);
static GeglRectangle   compute_affected_region (GeglOperation       *operation,
                                                const gchar         *input_pad,
                                                const GeglRectangle *input_region);
static void            get_property            (GObject             *object,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);
static void            set_property            (GObject             *object,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);

G_DEFINE_DYNAMIC_TYPE (GeglOperationNop, gegl_operation_nop, GEGL_TYPE_OPERATION)

static void
gegl_operation_nop_class_init (GeglOperationNopClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->get_property             = get_property;
  object_class->set_property             = set_property;

  operation_class->categories            = "core";
  operation_class->no_cache              = TRUE;
  operation_class->process               = process;
  operation_class->attach                = attach;
  operation_class->detect                = detect;
  operation_class->get_defined_region    = get_defined_region;
  operation_class->compute_input_request = compute_input_request;
  operation_class->compute_affected_region = compute_affected_region;

  gegl_operation_class_set_name (operation_class, "nop");


  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
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

}

static void
gegl_operation_nop_class_finalize (GeglOperationNopClass *klass)
{
}

static void
gegl_operation_nop_init (GeglOperationNop *self)
{
}

static void            get_property            (GObject             *object,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec)
{
}

static void            set_property            (GObject             *object,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec)
{
}

static void
attach (GeglOperation *operation)
{
  GObjectClass *object_class = G_OBJECT_GET_CLASS (operation);

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
  GeglNode           *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation, x, y);

  return operation->node;
}


static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglBuffer *input;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a nop", output_prop);
      return FALSE;
    }

  input  = gegl_node_context_get_source (context, "input");
  if (!input)
    {
      g_warning ("nop received NULL input");
      return FALSE;
    }

  gegl_node_context_set_object (context, "output", G_OBJECT (input));
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_defined_region (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  return *input_region;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  return *roi;
}

static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION, "nop", "", ""
};

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  gegl_operation_nop_register_type (module);

  return TRUE;
}
