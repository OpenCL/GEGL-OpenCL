#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-visitable.h"

#include "gegl-mock-node-visitor.h"


static void gegl_mock_node_visitor_class_init (GeglMockNodeVisitorClass *klass);
static void gegl_mock_node_visitor_init       (GeglMockNodeVisitor      *self);

static void visit_node (GeglVisitor *visitor,
                        GeglNode    *node);


G_DEFINE_TYPE (GeglMockNodeVisitor, gegl_mock_node_visitor, GEGL_TYPE_VISITOR)


static void
gegl_mock_node_visitor_class_init (GeglMockNodeVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_mock_node_visitor_init (GeglMockNodeVisitor *self)
{
}

static void
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  GEGL_VISITOR_CLASS (gegl_mock_node_visitor_parent_class)->visit_node (visitor,
                                                                        node);
#if 0
  g_print ("Visiting node %s\n", gegl_object_get_name(GEGL_OBJECT(node)));
#endif
}
