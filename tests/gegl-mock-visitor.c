#include "gegl-mock-visitor.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-filter.h"

static void class_init (GeglMockVisitorClass * klass);

static void visit_node (GeglVisitor *visitor, GeglNode * node);
static void visit_op (GeglVisitor *visitor, GeglOp * op);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_mock_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockVisitor),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglMockVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
  visitor_class->visit_op = visit_op;
  visitor_class->visit_filter = visit_filter;
}

static void      
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);
}

static void      
visit_op(GeglVisitor * visitor,
         GeglOp *op)
{
  visit_node(visitor, GEGL_NODE(op));
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  visit_node(visitor, GEGL_NODE(filter));
}
