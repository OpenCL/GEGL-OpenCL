/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-operation.h"
#include "gegl-property.h"


static void gegl_operation_class_init (GeglOperationClass *klass);
static void gegl_operation_init       (GeglOperation      *self);
static void finalize                  (GObject         *gobject);


G_DEFINE_TYPE(GeglOperation, gegl_operation, GEGL_TYPE_NODE)

static void
gegl_operation_class_init (GeglOperationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
}

static void
gegl_operation_init (GeglOperation *self)
{
}

static void
finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gegl_operation_parent_class)->finalize (gobject);
}

/**
 * gegl_operation_create_property:
 * @self: a #GeglOperation.
 * @param_spec:
 *
 * Create a property.
 **/
void
gegl_operation_create_property (GeglOperation *self,
                                GParamSpec    *param_spec)
{
  GeglProperty *property;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (param_spec);

  property = g_object_new (GEGL_TYPE_PROPERTY, NULL);
  gegl_property_set_param_spec (property, param_spec);
  gegl_property_set_operation (property, self);
  gegl_node_add_property (GEGL_NODE (self), property);
}

gboolean
gegl_operation_evaluate (GeglOperation  *self,
                         const gchar    *output_prop)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (self);

  return klass->evaluate (self, output_prop);
}
