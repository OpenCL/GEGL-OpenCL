#include "gegl-bfs-visitor.h"

static void class_init (GeglBfsVisitorClass * klass);
static void init (GeglBfsVisitor * self, GeglBfsVisitorClass * klass);

static void init_traversal (GeglBfsVisitor * self, GeglNode * node); 
static gpointer parent_class = NULL;

GType
gegl_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglBfsVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglBfsVisitorClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglBfsVisitor * self, 
      GeglBfsVisitorClass * klass)
{
}

static void 
init_traversal (GeglBfsVisitor * self, 
                GeglNode * node) 
{
  GeglVisitor * visitor;
  gint num_inputs;
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BFS_VISITOR (self));
  g_return_if_fail (node);
  g_return_if_fail (GEGL_IS_NODE (node));
   
  visitor = GEGL_VISITOR(self);

  gegl_visitor_node_insert(visitor, node);

  num_inputs = gegl_node_get_num_inputs(node);

  for(i = 0; i < num_inputs; i++)
    {
      GeglNode *source = gegl_node_get_source(node, i);

      if(source)
        {
          gint shared_count;

          GeglNodeInfo * node_info = 
            gegl_visitor_node_lookup(visitor, source); 
          if(!node_info)
           init_traversal(self, source);

          shared_count = gegl_visitor_get_shared_count(visitor, source); 
          shared_count++;
          gegl_visitor_set_shared_count(visitor, 
                                        source, 
                                        shared_count);
        }
    }
}

/**
 * gegl_bfs_visitor_traverse:
 * @self: a #GeglBfsVisitor
 * @node: the root #GeglNode.
 *
 * Traverse the graph breadth-first starting at @node. 
 *
 **/
void 
gegl_bfs_visitor_traverse(GeglBfsVisitor *self, 
                          GeglNode * node) 
{
  GList *queue = NULL; 
  GList *first;
  gint shared_count;
  GeglVisitor * visitor;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BFS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE (node));

  visitor = GEGL_VISITOR(self);
 
  /* Init all nodes */
  init_traversal(self, node);  

  /* Initialize the queue with this node */
  queue = g_list_append(queue,(gpointer)node);
  
  /* Mark node as "discovered" */
  gegl_visitor_set_discovered(visitor, node, TRUE);

  /* Pop the top of the queue*/
  while((first = g_list_first(queue)))
    {

      GeglNode * node = (GeglNode *)(first->data);

      queue = g_list_remove_link(queue,first);
      g_list_free_1(first);

      /* Put this one at the end of the queue, if its
         active immediate parents havent all been visited yet */
      shared_count = gegl_visitor_get_shared_count(visitor, node);
      if(shared_count > 0)
        {
          queue = g_list_append(queue,node);
          continue;
        }

      /* Loop through node's sources and examine them */
      {
        gint i;
        gint num_inputs = gegl_node_get_num_inputs(node);

        for(i = 0; i < num_inputs; i++)
          {
            GeglNode *source = gegl_node_get_source(node, i);
            if(source) 
              {
                shared_count = gegl_visitor_get_shared_count(visitor,source);
                shared_count--;
                gegl_visitor_set_shared_count(visitor, source, shared_count);
              }
            
            /* Add any undiscovered source to the queue at end,
               but skip any null nodes */
            if (source)
              {
                gint discovered = gegl_visitor_get_discovered(visitor, source);
                if(!discovered) 
                  {
                    queue = g_list_append(queue, (gpointer)source);

                    /* Mark it as discovered */
                    gegl_visitor_set_discovered(visitor, source, TRUE);
                  }
              }
          }
      }

      /* Visit the node */
      gegl_node_accept(node, visitor);
      gegl_visitor_set_visited(visitor, node, TRUE);
    }
}
