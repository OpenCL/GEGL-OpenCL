#include "gegl-graph.h"
#include "gegl-node.h"
#include "gegl-visitor.h"
#include "gegl-graph-setup-visitor.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_ROOT, 
  PROP_LAST 
};

static void class_init (GeglGraphClass * klass);
static void init (GeglGraph * self, GeglGraphClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void init_attributes(GeglOp *op);
static void free_list(GeglGraph *self, GList *list);

static void reset_inputs(GeglGraph *self);
static void reset_outputs(GeglGraph *self);

static void accept (GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

GType
gegl_graph_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglGraphClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglGraph),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglGraph", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglGraphClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);
  GeglOpClass *op_class = GEGL_OP_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;
  op_class->init_attributes = init_attributes;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this graph",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglGraph * self, 
      GeglGraphClass * klass)
{
  self->root = NULL;
  return;
}

static void
finalize(GObject *gobject)
{  
  GeglGraph *self = GEGL_GRAPH(gobject);

  free_list(self, self->graph_inputs);
  free_list(self, self->graph_outputs);

  if(self->root)
    g_object_unref(self->root);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
free_list(GeglGraph *self,
          GList *list)
{
  GList *llink = list;
  while(llink)
    {
      gpointer data = llink->data;
      g_free(data);
      llink = g_list_next(llink);
    }

  g_list_free(list);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglGraph * graph = GEGL_GRAPH(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
        gegl_graph_set_root(graph, (GeglNode*)g_value_get_object(value));  
      break;
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglGraph * graph = GEGL_GRAPH(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
      g_value_set_object(value, (GObject*)gegl_graph_get_root(graph));  
      break;
    default:
      break;
  }
}

/**
 * gegl_graph_get_root:
 * @self: a #GeglGraph.
 *
 * Gets the root node of this graph.
 *
 * Returns: a #GeglNode, the root of this graph.
 **/
GeglNode* 
gegl_graph_get_root (GeglGraph * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return self->root;
}

/**
 * gegl_graph_set_root:
 * @self: a #GeglGraph.
 * @root: a #GeglNode.
 *
 * Sets the root node of this graph.
 *
 **/
void 
gegl_graph_set_root (GeglGraph * self,
                     GeglNode *root)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_GRAPH (self));
  g_return_if_fail (GEGL_IS_NODE (root));

  if(self->root)
    g_object_unref(self->root);

  self->root = root;

  if(self->root)
    g_object_ref(self->root);

  reset_inputs(self);
  reset_outputs(self);
}

static void
reset_inputs(GeglGraph *self)
{
  GeglGraphSetupVisitor * graph_setup_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_SETUP_VISITOR, NULL); 

  graph_setup_visitor->graph = self;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_setup_visitor), 
                            self->root);

  if(self->graph_inputs)
    free_list(self, self->graph_inputs);
   
  self->graph_inputs = graph_setup_visitor->graph_inputs;
  g_object_unref(graph_setup_visitor);

  gegl_node_set_num_inputs(GEGL_NODE(self), 
                           g_list_length(self->graph_inputs));
}

static void
reset_outputs(GeglGraph *self)
{
  gint i;
  gint num_outputs = gegl_node_get_num_outputs(self->root);

  if(self->graph_outputs)
    free_list(self, self->graph_outputs);

  for(i = 0; i < num_outputs; i++)
  {
    GeglGraphOutput * graph_output = g_new(GeglGraphOutput, 1);
    graph_output->node = self->root;
    graph_output->node_output = i;
    graph_output->graph = self; 
    graph_output->graph_output = i;
    self->graph_outputs = g_list_append(self->graph_outputs, graph_output); 
  }

  g_object_set(self, "num_outputs", num_outputs, NULL);
}

GeglNode *
gegl_graph_find_source(GeglGraph *self,
                       gint * output,
                       GeglNode *node,
                       gint index)
{
  GeglNode * source;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  source = gegl_node_get_source(node, output, index);

  /* If the source is null, must get it from the graph */
  if(!source)
    {
      GeglGraphInput *graph_input = 
        gegl_graph_lookup_input(self, node, index); 

      source = gegl_node_get_source(GEGL_NODE(self), 
                                     output,
                                     graph_input->graph_input);
    }

  return source;
}

/**
 * gegl_graph_lookup_output:
 * @self: a #GeglGraph.
 * @graph_output: output index of graph. 
 *
 * Lookup the #GeglGraphOutput for this output index. 
 *
 * Returns: a #GeglGraphOutput.
 **/
GeglGraphOutput *
gegl_graph_lookup_output(GeglGraph *self,
                         gint graph_output)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return (GeglGraphOutput*)g_list_nth_data(self->graph_outputs, graph_output);
}

/**
 * gegl_graph_lookup_input:
 * @self: a #GeglGraph.
 * @node: a #GeglNode.
 * @node_input: source index of node. 
 *
 * Look for a #GeglGraphInput for this node. 
 *
 * Returns: a #GeglGraphInput or NULL if not found.
 **/
GeglGraphInput *
gegl_graph_lookup_input(GeglGraph *self,
                        GeglNode *node,
                        gint node_input)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (node, NULL);
  g_return_val_if_fail (GEGL_IS_NODE(node), NULL);

  {
    gint j;
    gint num_graph_inputs = g_list_length(self->graph_inputs);

    for(j=0; j < num_graph_inputs; j++) 
      {
         GeglGraphInput * graph_input = g_list_nth_data(self->graph_inputs,j); 

         if(graph_input->node == node && 
            graph_input->node_input == node_input) 
             return graph_input;
      }
  }

  return NULL; 
}

static void
init_attributes(GeglOp *op)
{
  gint i;
  GeglGraph *self = GEGL_GRAPH(op);
  gint num_output_values = 
    gegl_node_get_num_outputs(GEGL_NODE(op));

  g_return_if_fail (op != NULL);
  g_return_if_fail (GEGL_IS_GRAPH(op));
  g_return_if_fail (num_output_values >= 0);

  /* This will allocate the array of attributes pointers. */
  GEGL_OP_CLASS(parent_class)->init_attributes(op);

   /* Now we set each attribute pointer to the corresponding real one. */
  for(i = 0; i < num_output_values; i++)
    {
      GeglGraphOutput *graph_output = 
        (GeglGraphOutput*)g_list_nth_data(self->graph_outputs, i);

      if(GEGL_IS_OP(graph_output->node))
       op->attributes[i] = 
        gegl_op_get_nth_attributes(GEGL_OP(graph_output->node), 
                                   graph_output->node_output);
      else
       op->attributes[i] = NULL; 
    }
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_graph(visitor, GEGL_GRAPH(node));
}
