#include "gegl-dfs-visitor.h"
#include "gegl-attributes.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-graph.h"

static void class_init (GeglDfsVisitorClass * klass);
static void init (GeglDfsVisitor * self, GeglDfsVisitorClass * klass);

static void init_traversal (GeglDfsVisitor * self, GeglNode * node);
static void dfs_visitor_traverse(GeglDfsVisitor * self, GeglNode * node); 

static gpointer parent_class = NULL;

GType
gegl_dfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglDfsVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglDfsVisitorClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglDfsVisitor * self, 
      GeglDfsVisitorClass * klass)
{
}

static void 
init_traversal (GeglDfsVisitor * self,
                GeglNode * node)
{
  GeglVisitor * visitor;
  gint num_inputs;
  gint i;

  g_return_if_fail (self);
  g_return_if_fail (GEGL_IS_DFS_VISITOR (self));
  g_return_if_fail (node);
  g_return_if_fail (GEGL_IS_NODE (node));

  visitor = GEGL_VISITOR(self);
   
  gegl_visitor_node_insert(visitor, node);

  num_inputs = gegl_node_get_num_inputs(node);
  for(i = 0; i < num_inputs; i++)
    {
      gint output;
      GeglNode *source = gegl_node_get_source(node, &output, i);

      if(source)
        {
          GeglNodeInfo * node_info = 
            gegl_visitor_node_lookup(visitor, source);

          if(!node_info)
           init_traversal(self, source);
        }
    }
}

/**
 * gegl_dfs_visitor_traverse:
 * @self: #GeglDfsVisitor
 * @node: the root #GeglNode
 *
 * Traverse the graph depth first.
 *
 **/
void 
gegl_dfs_visitor_traverse(GeglDfsVisitor * self,
                          GeglNode * node) 
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_DFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  init_traversal(self, node);
  dfs_visitor_traverse(self, node);
}

static void 
dfs_visitor_traverse(GeglDfsVisitor * self,
                     GeglNode * node) 
{
  GeglVisitor *visitor;
  gint i;
  gint num_inputs;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_DFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  visitor = GEGL_VISITOR(self);

  num_inputs = gegl_node_get_num_inputs(node);
  for(i = 0; i < num_inputs; i++)
    {
      gint output;
      GeglNode *source = gegl_node_get_source(node, &output, i);
      if(source) 
        {
         gboolean visited = gegl_visitor_get_visited(visitor, source);

         if(!visited) 
          dfs_visitor_traverse(self, source);
        }
    }

  /* Visit this node last */
  gegl_node_accept(node, visitor);
  gegl_visitor_set_visited(visitor, node, TRUE);
}
