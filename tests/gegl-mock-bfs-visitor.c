#include "gegl-mock-bfs-visitor.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-graph.h"

static void class_init (GeglMockBfsVisitorClass * klass);

static void visit_node (GeglVisitor *visitor, GeglNode * node);
static void visit_filter (GeglVisitor *visitor, GeglFilter * op);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_mock_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockBfsVisitor),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GEGL_TYPE_BFS_VISITOR, 
                                     "GeglMockBfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockBfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void      
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *op)
{
  visit_node(visitor, GEGL_NODE(op));
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  visit_node(visitor, GEGL_NODE(graph));
  gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(visitor), graph->root);
}
