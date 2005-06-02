#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-visitable.h"

#include "gegl-mock-node-visitor.h"


static void class_init (GeglMockNodeVisitorClass * klass);
static void visit_node (GeglVisitor *visitor, GeglNode * node);

static gpointer parent_class = NULL;

GType
gegl_mock_node_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockNodeVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockNodeVisitor),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR,
                                     "GeglMockNodeVisitor",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GeglMockNodeVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
}

static void
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);
  {
#if 0
    g_print("Visiting node %s\n", gegl_object_get_name(GEGL_OBJECT(node)));
#endif
  }
}
