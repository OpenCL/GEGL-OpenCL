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

#include "gegl-eval-visitor.h"
#include "gegl-node.h"
#include "gegl-property.h"
#include "gegl-visitable.h"

static void class_init (GeglEvalVisitorClass * klass);
static void visit_property (GeglVisitor *visitor, GeglProperty * property);

static gpointer parent_class = NULL;

GType
gegl_eval_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalVisitor),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglEvalVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_property = visit_property;
}

static void      
visit_property(GeglVisitor * visitor,
               GeglProperty *property)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_property(visitor, property);

  {
    GeglFilter *filter = gegl_property_get_filter(property);
#if 0
    g_print("Compute Visitor: Visiting property %s from filter %s\n", 
             gegl_property_get_name(property),
             gegl_object_get_name(GEGL_OBJECT(filter)));
#endif

    if(gegl_property_is_output(property))
      {
        gboolean success;
        const gchar *property_name = gegl_property_get_name(property);
        success = gegl_filter_evaluate(filter, property_name);
      }
    else if(gegl_property_is_input(property))
      {
        GeglProperty *source_prop = gegl_property_get_connected_to(property);
        if(source_prop)
          {
            GValue value = {0};
            GParamSpec *prop_spec = gegl_property_get_param_spec(property);
            GeglFilter *source = gegl_property_get_filter(source_prop);

            g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(prop_spec));

            g_object_get_property(G_OBJECT(source), 
                                  gegl_property_get_name(source_prop), 
                                  &value);
            g_object_set_property(G_OBJECT(filter), 
                                  gegl_property_get_name(property), 
                                  &value); 

          }
      }
  }
}
