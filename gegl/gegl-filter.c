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

#include "gegl-filter.h"
#include "gegl-property.h"
#include <string.h>

static void class_init (GeglFilterClass * klass);
static void init (GeglFilter * self, GeglFilterClass * klass);
static void finalize(GObject * gobject);

static gpointer parent_class = NULL;

GType
gegl_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilter),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_NODE , 
                                     "GeglFilter", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
}

static void 
init (GeglFilter * self, 
      GeglFilterClass * klass)
{
}

static void
finalize(GObject *gobject)
{
  GeglFilter *self = GEGL_FILTER(gobject);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_filter_create_property:
 * @self: a #GeglFilter.
 * @name: property name.
 *
 * Create a property.
 *
 **/
void
gegl_filter_create_property(GeglFilter *self,
                        GParamSpec *param_spec)
{
  GeglProperty * property; 
  g_return_if_fail(GEGL_IS_FILTER(self));
  g_return_if_fail(param_spec);

  property = g_object_new (GEGL_TYPE_PROPERTY, NULL); 
  gegl_property_set_param_spec(property, param_spec);
  gegl_property_set_filter(property, self);
  gegl_node_add_property(GEGL_NODE(self), property);
}

gboolean           
gegl_filter_evaluate (GeglFilter *self, 
                      const gchar *output_prop)
{
  GeglFilterClass *klass;
  g_return_val_if_fail (GEGL_IS_FILTER (self), FALSE);

  klass = GEGL_FILTER_GET_CLASS(self);
  return klass->evaluate(self, output_prop);
}
