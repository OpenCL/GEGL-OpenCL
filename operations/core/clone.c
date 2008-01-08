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

#ifndef __GEGL_OPERATION_CLONE_H__
#define __GEGL_OPERATION_CLONE_H__

#include "operation/gegl-operation.h"
#include "gegl-module.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_CLONE            (gegl_operation_clone_get_type ())
#define GEGL_OPERATION_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_CLONE, GeglOperationClone))
#define GEGL_OPERATION_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_CLONE, GeglOperationCloneClass))
#define GEGL_IS_OPERATION_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_CLONE))
#define GEGL_IS_OPERATION_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_CLONE))
#define GEGL_OPERATION_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_CLONE, GeglOperationCloneClass))

typedef struct _GeglOperationClone      GeglOperationClone;
typedef struct _GeglOperationCloneClass GeglOperationCloneClass;

struct _GeglOperationClone
{
  GeglOperation parent_instance;
  gchar *ref;
};

struct _GeglOperationCloneClass
{
  GeglOperationClass parent_class;
};

GType gegl_operation_clone_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEGL_OPERATION_CLONE_H__ */

/***************************************************************************/

#include "graph/gegl-pad.h"
#include "graph/gegl-node.h"
#include <math.h>
#include <string.h>


enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_REF
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

G_DEFINE_DYNAMIC_TYPE (GeglOperationClone, gegl_operation_clone, GEGL_TYPE_OPERATION)

static void
gegl_operation_clone_class_init (GeglOperationCloneClass *klass)
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

  gegl_operation_class_set_name (operation_class, "clone");


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

  g_object_class_install_property (object_class, PROP_REF,
                                   g_param_spec_string ("ref",
                                                        "Reference",
                                                        "The reference ID used as input (for use in XML).",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));
}

static void
gegl_operation_clone_class_finalize (GeglOperationCloneClass *klass)
{
}

static void
gegl_operation_clone_init (GeglOperationClone *self)
{
  self->ref = NULL;
}

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglOperationClone *self = GEGL_OPERATION_CLONE (object);

  switch (property_id)
    {
    case PROP_REF:
      g_value_set_string (value, self->ref);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglOperationClone *self = GEGL_OPERATION_CLONE (object);

  switch (property_id)
    {
    case PROP_REF:
      if (self->ref)
        g_free (self->ref);
      self->ref = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
  GeglNode *node = operation->node;

  if (x>=node->have_rect.x &&
      y>=node->have_rect.y &&
      x<node->have_rect.width  &&
      y<node->have_rect.height )
    {
      return node;
    }
  return NULL;
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
      g_warning ("requested processing of %s pad on a clone", output_prop);
      return FALSE;
    }

  input  = gegl_node_context_get_source (context, "input");
  if (!input)
    {
      g_warning ("clone received NULL input");
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
  GEGL_MODULE_ABI_VERSION, "clone", "", ""
};

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  gegl_operation_clone_register_type (module);

  return TRUE;
}
