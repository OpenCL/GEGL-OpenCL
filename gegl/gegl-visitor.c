#include "gegl-visitor.h"
#include "gegl-node.h"
#include "gegl-filter.h"
#include "gegl-graph.h"

static void class_init (GeglVisitorClass * klass);
static void init (GeglVisitor * self, GeglVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_node(GeglVisitor * self, GeglNode *node);
static void visit_op(GeglVisitor * self, GeglOp *op);
static void visit_filter(GeglVisitor * self, GeglFilter *filter);
static void visit_graph(GeglVisitor * self, GeglGraph *op);

static void node_info_value_destroy(gpointer data);

static gpointer parent_class = NULL;

GType
gegl_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglVisitor),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglVisitorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->visit_node = visit_node; 
  klass->visit_filter = visit_filter; 
  klass->visit_op = visit_op; 
  klass->visit_graph = visit_graph; 
}

static void 
init (GeglVisitor * self, 
      GeglVisitorClass * klass)
{
  self->graph = NULL;
  self->visits_list = NULL;
  self->nodes_hash = g_hash_table_new_full(g_direct_hash,
                                           g_direct_equal,
                                           NULL,
                                           node_info_value_destroy); 
}

static void
finalize(GObject *gobject)
{  
  GeglVisitor * self = GEGL_VISITOR(gobject);

  g_list_free(self->visits_list);
  g_hash_table_destroy(self->nodes_hash);
     
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_visitor_node_lookup
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Look up the node info for this node.
 *
 * Returns: a #GeglNodeInfo for the node if found.
 **/
GeglNodeInfo *
gegl_visitor_node_lookup(GeglVisitor * self,
                         GeglNode *node)
{
  GeglNodeInfo * node_info;
  g_return_val_if_fail(GEGL_IS_VISITOR(self), NULL);
  g_return_val_if_fail(GEGL_IS_NODE(node), NULL);

  node_info = g_hash_table_lookup(self->nodes_hash, node);
  return node_info;
}

/**
 * gegl_visitor_node_insert
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Insert the node into the nodes hashtable of this visitor.
 *
 **/
void
gegl_visitor_node_insert(GeglVisitor *self,
                         GeglNode *node)
{
  GeglNodeInfo * node_info;
  g_return_if_fail(GEGL_IS_VISITOR(self));
  g_return_if_fail(GEGL_IS_NODE(node));

  node_info = gegl_visitor_node_lookup(self,node);

  if(!node_info)
    {
      node_info = g_new(GeglNodeInfo, 1);
      node_info->visited = FALSE;
      node_info->discovered = FALSE;
      node_info->shared_count = 0;
      g_hash_table_insert(self->nodes_hash, node, node_info);
    }
  else
    {
      g_warning("Node already in nodes hash table");
    }
}

/**
 * gegl_visitor_get_visited
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Gets whether this node has been visited by this visitor.
 *
 * Returns: whether this node has been visited. 
 **/
gboolean
gegl_visitor_get_visited(GeglVisitor *self,
                         GeglNode *node)
{
  GeglNodeInfo * node_info;
  g_return_val_if_fail(GEGL_IS_VISITOR(self), FALSE);
  g_return_val_if_fail(GEGL_IS_NODE(node), FALSE);

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->visited;
}

/**
 * gegl_visitor_set_visited
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 * @visited: a boolean.
 *
 * Sets whether this node has been visited by this visitor.
 *
 **/
void
gegl_visitor_set_visited(GeglVisitor *self,
                         GeglNode *node,
                         gboolean visited)
{
  GeglNodeInfo * node_info;
  g_return_if_fail(GEGL_IS_VISITOR(self));
  g_return_if_fail(GEGL_IS_NODE(node));

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->visited = visited;
}

/**
 * gegl_visitor_get_discovered
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Gets whether this node has been discovered by this visitor.
 *
 * Returns: whether this node has been discovered. 
 **/
gboolean
gegl_visitor_get_discovered(GeglVisitor *self,
                            GeglNode *node)
{
  GeglNodeInfo * node_info;
  g_return_val_if_fail(self, FALSE);
  g_return_val_if_fail(node, FALSE);

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->discovered;
}

/**
 * gegl_visitor_set_discovered
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 * @discovered: a boolean.
 *
 * Sets whether this node has been discovered by this visitor.
 *
 **/
void
gegl_visitor_set_discovered(GeglVisitor *self,
                            GeglNode *node,
                            gboolean discovered)
{
  GeglNodeInfo * node_info;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->discovered = discovered;
}

/**
 * gegl_visitor_get_shared_count
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Gets the shared count of this node. This is the number of parents that have
 * not yet been visited by the visitor.
 *
 * Returns: number of parents not yet visited  
 **/
gint
gegl_visitor_get_shared_count(GeglVisitor *self,
                              GeglNode *node)
{
  GeglNodeInfo * node_info;
  g_return_val_if_fail (GEGL_IS_VISITOR (self), -1);
  g_return_val_if_fail (GEGL_IS_NODE (node), -1);

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->shared_count;
}

/**
 * gegl_visitor_set_shared_count
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 * @shared_count: the shared count.
 *
 * Sets the shared count of this node. This is the number of parents that have
 * not yet been visited by the visitor.
 *
 **/
void
gegl_visitor_set_shared_count(GeglVisitor *self,
                              GeglNode *node,
                              gint shared_count)
{
  GeglNodeInfo * node_info;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  node_info->shared_count = shared_count;
}

/**
 * gegl_visitor_get_visits_list
 * @self: a #GeglVisitor.
 *
 * Gets a list of the nodes the visitor has visited so far. 
 *
 * Returns: A list of the nodes visited by this visitor. 
 **/
GList *
gegl_visitor_get_visits_list(GeglVisitor *self)
{
  g_return_val_if_fail (GEGL_IS_VISITOR (self), NULL);

  return self->visits_list;
}

static void
node_info_value_destroy(gpointer data)
{
  GeglNodeInfo *node_info = (GeglNodeInfo *)data;
  g_free(node_info);
}

/**
 * gegl_visitor_visit_node
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Visit the node. 
 *
 **/
void      
gegl_visitor_visit_node(GeglVisitor * self,
                        GeglNode *node)
{
  GeglVisitorClass *klass;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_NODE(node));

  klass = GEGL_VISITOR_GET_CLASS(self);
  if(klass->visit_node)
    (*klass->visit_node)(self, node);
}

static void      
visit_node(GeglVisitor * self,
           GeglNode *node)
{
 self->visits_list = g_list_append(self->visits_list, node);
}

/**
 * gegl_visitor_visit_filter
 * @self: a #GeglVisitor.
 * @filter: a #GeglFilter.
 *
 * Visit the filter. 
 *
 **/
void      
gegl_visitor_visit_filter(GeglVisitor * self,
                          GeglFilter *filter)
{
  GeglVisitorClass *klass;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_FILTER(filter));

  klass = GEGL_VISITOR_GET_CLASS(self);
  if(klass->visit_filter)
    (*klass->visit_filter)(self, filter);
}

static void      
visit_filter(GeglVisitor * self,
             GeglFilter *filter)
{
  visit_node(self, GEGL_NODE(filter));
}

/**
 * gegl_visitor_visit_op
 * @self: a #GeglVisitor.
 * @op: a #GeglOp.
 *
 * Visit the op. 
 *
 **/
void      
gegl_visitor_visit_op(GeglVisitor * self,
                      GeglOp *op)
{
  GeglVisitorClass *klass;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_OP(op));

  klass = GEGL_VISITOR_GET_CLASS(self);
  if(klass->visit_op)
    (*klass->visit_op)(self, op);
}

static void      
visit_op(GeglVisitor * self,
         GeglOp *op)
{
  visit_node(self, GEGL_NODE(op));
}

/**
 * gegl_visitor_visit_graph
 * @self: a #GeglVisitor.
 * @graph: a #GeglGraph.
 *
 * Visit the graph. 
 *
 **/
void      
gegl_visitor_visit_graph(GeglVisitor * self,
                          GeglGraph *graph)
{
  GeglVisitorClass *klass;
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (GEGL_IS_GRAPH(graph));

  klass = GEGL_VISITOR_GET_CLASS(self);
  if(klass->visit_graph)
    (*klass->visit_graph)(self, graph);
}

static void      
visit_graph(GeglVisitor * self,
             GeglGraph *graph)
{
  visit_node(self, GEGL_NODE(graph));
}

/**
 * gegl_visitor_collect_input_data
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * This routine finds the data inputs to be delivered to a node. The returned
 * array should be freed when finished.
 *
 * Returns: an array of data.
 **/
GArray * 
gegl_visitor_collect_input_data(GeglVisitor *self,
                                GeglNode *node)
{
  GArray * input_data_array;
  gint i;
  gint num_inputs;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE(node), NULL);

  input_data_array = g_array_new(FALSE, FALSE, sizeof(GeglData*));
  num_inputs = gegl_node_get_num_inputs(node);

  /* There is a bug here, if we dont find the inputs 
     one level out, we have to climb out further. I 
     guess we need a stack of levels (Graphs) the visitor 
     is inside of, not just the current one we are inside. 
   */
  for(i = 0 ; i < num_inputs; i++) 
    {
      GeglNode *source = gegl_node_get_source(node, i);

      /* Try the graph instead. */
      if(!source && self->graph) 
        source = gegl_graph_find_source(self->graph, node, i); 

      if(source) 
        {
          GeglData *input_data = gegl_op_get_nth_output_data(GEGL_OP(source), 0); 
          input_data_array = g_array_append_val(input_data_array, input_data);
        }
      else
        {
          GeglData *input_data = NULL;
          input_data_array = g_array_append_val(input_data_array, input_data);
        }

    }

  return input_data_array;
}
