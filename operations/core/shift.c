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

#ifndef __GEGL_OPERATION_SHIFT_H__
#define __GEGL_OPERATION_SHIFT_H__

#include "operation/gegl-operation.h"
#include "gegl-module.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_SHIFT            (gegl_operation_shift_get_type ())
#define GEGL_OPERATION_SHIFT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_SHIFT, GeglOperationShift))
#define GEGL_OPERATION_SHIFT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_SHIFT, GeglOperationShiftClass))
#define GEGL_IS_OPERATION_SHIFT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_SHIFT))
#define GEGL_IS_OPERATION_SHIFT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_SHIFT))
#define GEGL_OPERATION_SHIFT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_SHIFT, GeglOperationShiftClass))

typedef struct _GeglOperationShift      GeglOperationShift;
typedef struct _GeglOperationShiftClass GeglOperationShiftClass;

struct _GeglOperationShift
{
  GeglOperation parent_instance;

  gdouble       x;
  gdouble       y;
};

struct _GeglOperationShiftClass
{
  GeglOperationClass parent_class;
};

GType gegl_operation_shift_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEGL_OPERATION_SHIFT_H__ */

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
  PROP_X,
  PROP_Y
};

static void            get_property            (GObject             *object,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);
static void            set_property            (GObject             *object,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);

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

G_DEFINE_DYNAMIC_TYPE (GeglOperationShift, gegl_operation_shift,
                       GEGL_TYPE_OPERATION)

static void
gegl_operation_shift_class_init (GeglOperationShiftClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property             = set_property;
  object_class->get_property             = get_property;

  operation_class->categories            = "core";
  operation_class->no_cache              = TRUE;
  operation_class->process               = process;
  operation_class->attach                = attach;
  operation_class->detect                = detect;
  operation_class->get_defined_region    = get_defined_region;
  operation_class->compute_input_request = compute_input_request;
  operation_class->compute_affected_region = compute_affected_region;

  gegl_operation_class_set_name (operation_class, "shift");

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

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x",
                                                       "X",
                                                       "X",
                                                       -G_MAXDOUBLE, G_MAXDOUBLE,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y",
                                                       "Y",
                                                       "Y",
                                                       -G_MAXDOUBLE, G_MAXDOUBLE,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));


}

static void
gegl_operation_shift_class_finalize (GeglOperationShiftClass *klass)
{
}

static void
gegl_operation_shift_init (GeglOperationShift *self)
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
  GeglOperationShift *self = GEGL_OPERATION_SHIFT (operation);
  GeglNode           *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation,
                                  x - floor (self->x),
                                  y - floor (self->y));

  return operation->node;
}

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglOperationShift *self = GEGL_OPERATION_SHIFT (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_double (value, self->x);
      break;

    case PROP_Y:
      g_value_set_double (value, self->y);
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
  GeglOperationShift *self = GEGL_OPERATION_SHIFT (object);

  switch (property_id)
    {
    case PROP_X:
      self->x = g_value_get_double (value);
      break;

    case PROP_Y:
      self->y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglOperationShift *shift   = GEGL_OPERATION_SHIFT (operation);
  GeglBuffer         *input;
  gboolean            success = FALSE;

  shift->x = floor (shift->x);
  shift->y = floor (shift->y);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a shift", output_prop);
      return FALSE;
    }

  input  = gegl_node_context_get_source (context, "input");

  if (input != NULL)
    {
      GeglBuffer *output;

      output = g_object_new (GEGL_TYPE_BUFFER,
                             "provider",    input,
                             "shift-x",     (int)-shift->x,
                             "shift-y",     (int)-shift->y,
                             "abyss-width", -1,  /* turn of abyss
                                                    (relying on abyss
                                                    of source) */
                         NULL);

      gegl_node_context_set_object (context, "output", G_OBJECT (output));

      g_object_unref (input);

      success = TRUE;
    }
  else
    {
      if (!g_object_get_data (G_OBJECT (operation->node), "graph"))
        g_warning ("%s got %s",
                   gegl_node_get_debug_name (operation->node),
                   input==NULL?"input==NULL":"");
    }

  return success;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglOperationShift *self    = GEGL_OPERATION_SHIFT (operation);
  GeglRectangle       result  = { 0, 0, 0, 0 };
  GeglRectangle      *in_rect = gegl_operation_source_get_defined_region (operation, "input");

  if (!in_rect)
    return result;

  result = *in_rect;

  result.x += floor (self->x);
  result.y += floor (self->y);

  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  GeglOperationShift *self   = GEGL_OPERATION_SHIFT (operation);
  GeglRectangle       result = *input_region;

  result.x += floor (self->x);
  result.y += floor (self->y);

  return result;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  GeglOperationShift *self   = GEGL_OPERATION_SHIFT (operation);
  GeglRectangle       result = *roi;

  result.x -= floor (self->x);
  result.y -= floor (self->y);

  return result;
}

static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION, "shift", "", ""
};

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  gegl_operation_shift_register_type (module);

  return TRUE;
}
