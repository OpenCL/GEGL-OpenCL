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
static GeglConnection *get_inputs(GeglVisitor * self, GeglNode *node);

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

  klass->get_inputs = get_inputs;
}

static void 
init (GeglVisitor * self, 
      GeglVisitorClass * klass)
{
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
  g_return_val_if_fail(self, NULL);
  g_return_val_if_fail(node, NULL);

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
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);

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
 * Returns: a boolean
 **/
gboolean
gegl_visitor_get_visited(GeglVisitor *self,
                         GeglNode *node)
{
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
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
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
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
 * Returns: a boolean
 **/
gboolean
gegl_visitor_get_discovered(GeglVisitor *self,
                            GeglNode *node)
{
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
  g_assert(node_info);
  return node_info->discovered;
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
gegl_visitor_set_discovered(GeglVisitor *self,
                            GeglNode *node,
                            gboolean discovered)
{
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
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
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
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
  GeglNodeInfo * node_info = gegl_visitor_node_lookup(self,node);
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
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE(node));

  klass = GEGL_VISITOR_GET_CLASS(self);

  if(klass->visit_node)
    (*klass->visit_node)(self, node);
}

static void      
visit_node(GeglVisitor * self,
           GeglNode *node)
{
 gchar* name = (gchar*)gegl_object_get_name(GEGL_OBJECT(node));
 self->visits_list = g_list_append(self->visits_list, name);
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
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (filter != NULL);
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
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (op != NULL);
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
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (graph != NULL);
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
 * gegl_visitor_get_inputs
 * @self: a #GeglVisitor.
 * @node: a #GeglNode.
 *
 * Get the input connections for this node. Free after finishing.
 *
 * Returns: an array of #GeglConnection. 
 **/
GeglConnection *
gegl_visitor_get_inputs(GeglVisitor * self,
                        GeglNode *node)
{
  GeglVisitorClass *klass;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_VISITOR (self), NULL);
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE(node), NULL);

  klass = GEGL_VISITOR_GET_CLASS(self);

  if(klass->get_inputs)
    return (*klass->get_inputs)(self, node);
  else
    return NULL;
}

static GeglConnection* 
get_inputs (GeglVisitor *self,
            GeglNode *node)
{
  return gegl_node_get_input_connections(node);
}
