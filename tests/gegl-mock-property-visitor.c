#include "gegl-mock-property-visitor.h"
#include "gegl-node.h"
#include "gegl-property.h"
#include "gegl-visitable.h"

static void class_init (GeglMockPropertyVisitorClass * klass);
static void visit_property (GeglVisitor *visitor, GeglProperty * property);

static gpointer parent_class = NULL;

GType
gegl_mock_property_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockPropertyVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockPropertyVisitor),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglMockPropertyVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockPropertyVisitorClass * klass)
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
#if 0
    GeglFilter *filter = gegl_property_get_filter(property);
    g_print("Visiting property %s from op %s\n", 
             gegl_property_get_name(property),
             gegl_object_get_name(GEGL_OBJECT(op)));
#endif
  }
}
