#include "gil-bfs-visitor.h"
#include "gil-node.h"

static void class_init (GilBfsVisitorClass * klass);
static void init (GilBfsVisitor * self, GilBfsVisitorClass * klass);

static void init_traversal (GilBfsVisitor * self, GilNode * node); 
static gpointer parent_class = NULL;

GType
gil_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilBfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_VISITOR, 
                                     "GilBfsVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GilBfsVisitorClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GilBfsVisitor * self, 
      GilBfsVisitorClass * klass)
{
}

static void 
init_traversal (GilBfsVisitor * self, 
                GilNode * node) 
{
  GilVisitor * visitor;
  gint num_children;
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_BFS_VISITOR (self));
  g_return_if_fail (node);
  g_return_if_fail (GIL_IS_NODE (node));
   
  visitor = GIL_VISITOR(self);
  gil_visitor_node_insert(visitor, node);

  num_children = gil_node_get_num_children(node);

  for(i = 0; i < num_children; i++)
    {
      GilNode * child = gil_node_get_nth_child(node,i);
      if(child)
        {
          gint shared_count;

          GilNodeInfo * node_info =
            gil_visitor_node_lookup(visitor, child);

          if(!node_info)
           init_traversal(self, child);

          shared_count = gil_visitor_get_shared_count(visitor, child); 
          shared_count++;
          gil_visitor_set_shared_count(visitor, 
                                       child, 
                                       shared_count);
        }
    }
}

/**
 * gil_bfs_visitor_traverse:
 * @self: a #GilBfsVisitor
 * @node: a #GilNode.
 *
 * Traverse the graph breadth-first from this node through all descendants. 
 *
 **/
void 
gil_bfs_visitor_traverse(GilBfsVisitor *self, 
                         GilNode * node) 
{
  GList *queue = NULL; 
  GList *first;
  gint shared_count;
  GilVisitor * visitor;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_BFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GIL_IS_NODE (node));

  visitor = GIL_VISITOR(self);
 
  /* Init all nodes */
  init_traversal(self, node);  

  /* Initialize the queue with this node */
  queue = g_list_append(queue,(gpointer)node);
  
  /* Mark node as "discovered" */
  gil_visitor_set_discovered(visitor, node, TRUE);

  /* Pop the top of the queue*/
  while((first = g_list_first(queue)))
    {

      GilNode * node = (GilNode *)(first->data);

      queue = g_list_remove_link(queue,first);
      g_list_free_1(first);

      /* Put this one at the end of the queue, if its
         active immediate parents havent all been visited yet */
      shared_count = gil_visitor_get_shared_count(visitor, node);
      if(shared_count > 0)
        {
          queue = g_list_append(queue,node);
          continue;
        }

      /* Loop through node's children and examine them */
      {
        gint i;
        gint num_children = gil_node_get_num_children(node);

        for(i = 0; i < num_children; i++)
          {
            GilNode * child = gil_node_get_nth_child(node, i);
            if(child) 
              {
                shared_count = gil_visitor_get_shared_count(visitor, child);
                shared_count--;
                gil_visitor_set_shared_count(visitor, child, shared_count);
              }
            
            /* Add any undiscovered child to the queue at end,
               but skip any null nodes */
            if (child)
              {
                gint discovered = gil_visitor_get_discovered(visitor, child);
                if(!discovered) 
                  {
                    queue = g_list_append(queue, (gpointer)child);

                    /* Mark it as discovered */
                    gil_visitor_set_discovered(visitor, child, TRUE);
                  }
              }
          }
      }

      /* Visit the node */
      gil_node_accept(node, visitor);
      gil_visitor_set_visited(visitor, node, TRUE);
    }
}
