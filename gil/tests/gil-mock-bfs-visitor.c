#include "gil-mock-bfs-visitor.h"
#include "gil-node.h"

static void class_init (GilMockBfsVisitorClass * klass);

static void visit_node (GilVisitor *visitor, GilNode * node);

static gpointer parent_class = NULL;

GType
gil_mock_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilMockBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilMockBfsVisitor),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GIL_TYPE_BFS_VISITOR, 
                                     "GilMockBfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GilMockBfsVisitorClass * klass)
{
  GilVisitorClass *visitor_class = GIL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
}

static void      
visit_node(GilVisitor * visitor,
           GilNode *node)
{
  GIL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);
}
