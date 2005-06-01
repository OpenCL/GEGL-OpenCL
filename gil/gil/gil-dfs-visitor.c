#include "gil-dfs-visitor.h"
#include "gil-node.h"

static void class_init (GilDfsVisitorClass * klass);
static void init (GilDfsVisitor * self, GilDfsVisitorClass * klass);

static void init_traversal (GilDfsVisitor * self, GilNode * node);
static void dfs_visitor_traverse(GilDfsVisitor * self, GilNode * node);

static gpointer parent_class = NULL;

GType
gil_dfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilDfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilDfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_VISITOR,
                                     "GilDfsVisitor",
                                     &typeInfo,
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void
class_init (GilDfsVisitorClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void
init (GilDfsVisitor * self,
      GilDfsVisitorClass * klass)
{
}

/**
 * init_traversal:
 * @self:
 *
 **/
static void
init_traversal (GilDfsVisitor * self,
                GilNode * node)
{
  GilVisitor * visitor;
  gint num_children;
  gint i;

  g_return_if_fail (self);
  g_return_if_fail (GIL_IS_DFS_VISITOR (self));
  g_return_if_fail (node);
  g_return_if_fail (GIL_IS_NODE (node));

  visitor = GIL_VISITOR(self);

  gil_visitor_node_insert(visitor, node);
  num_children = gil_node_get_num_children(node);

  for(i = 0; i < num_children; i++)
    {
      GilNode *child = gil_node_get_nth_child(node, i);
      if(child)
        {
          GilNodeInfo * node_info =
            gil_visitor_node_lookup(visitor, child);

          if(!node_info)
           init_traversal(self, child);
        }
    }
}

/**
 * gil_dfs_visitor_traverse:
 * @self:
 * @node:
 *
 *
 **/
void
gil_dfs_visitor_traverse(GilDfsVisitor * self,
                          GilNode * node)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_DFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GIL_IS_NODE (node));

  init_traversal(self, node);
  dfs_visitor_traverse(self, node);
}

static void
dfs_visitor_traverse(GilDfsVisitor * self,
                     GilNode * node)
{
  GilVisitor *visitor;
  gint i;
  gint num_children;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_DFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GIL_IS_NODE (node));

  visitor = GIL_VISITOR(self);
  num_children = gil_node_get_num_children(node);

  for(i = 0; i < num_children; i++)
    {
      GilNode *child = gil_node_get_nth_child(node, i);
      if(child)
        {
         gboolean visited = gil_visitor_get_visited(visitor, child);

         if(!visited)
          dfs_visitor_traverse(self, child);
        }
    }

  /* Visit this node last */
  gil_node_accept(node, visitor);
  gil_visitor_set_visited(visitor, node, TRUE);
}
