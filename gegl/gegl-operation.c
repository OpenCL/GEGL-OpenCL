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

#include "gegl-filter.h"
#include "gegl-property.h"

static void gegl_filter_class_init (GeglFilterClass *klass);
static void gegl_filter_init       (GeglFilter      *self);
static void finalize               (GObject         *gobject);


G_DEFINE_TYPE(GeglFilter, gegl_filter, GEGL_TYPE_NODE)

static void
gegl_filter_class_init (GeglFilterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
}

static void
gegl_filter_init (GeglFilter *self)
{
}

static void
finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gegl_filter_parent_class)->finalize (gobject);
}

/**
 * gegl_filter_create_property:
 * @self: a #GeglFilter.
 * @param_spec:
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
