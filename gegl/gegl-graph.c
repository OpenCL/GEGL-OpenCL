#include "gegl-graph.h"
#include "gegl-node.h"
#include "gegl-visitor.h"
#include "gegl-input-value-types.h"
#include "gegl-input-param-specs.h"
#include "gegl-graph-setup-visitor.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_ROOT, 
  PROP_INPUT,
  PROP_LAST 
};

static void class_init (GeglGraphClass * klass);
static void init (GeglGraph * self, GeglGraphClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void free_list(GeglGraph *self, GList *list);

static void root_changed(GeglGraph *self);
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
        NULL
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

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this graph",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INPUT,
                                   gegl_param_spec_input ("input",
                                                          "Input",
                                                          "Input for graph",
                                                          G_PARAM_WRITABLE));

}

static void 
init (GeglGraph * self, 
      GeglGraphClass * klass)
{
  self->root = NULL;
}

static void
finalize(GObject *gobject)
{  
  GeglGraph *self = GEGL_GRAPH(gobject);

  free_list(self, self->graph_inputs);

  if(self->root)
    g_object_unref(self->root);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglGraph * self = GEGL_GRAPH(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
        gegl_graph_set_root(self, (GeglNode*)g_value_get_object(value));  
      break;
    case PROP_INPUT:
      {
        gint n;
        GeglNode *source = g_value_get_input(value, &n); 
        gegl_node_set_source(GEGL_NODE(self), source, n);  
      }
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
  GeglGraph * self = GEGL_GRAPH(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
      g_value_set_object(value, (GObject*)gegl_graph_get_root(self));  
      break;
    default:
      break;
  }
}

static void
root_changed(GeglGraph * self)
{
  gegl_op_free_output_data_array(GEGL_OP(self));
  gegl_op_free_input_data_array(GEGL_OP(self));

  if(self->root)
    {
      reset_inputs(self);
      reset_outputs(self);
    }
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

  root_changed(self);
}

static void
reset_inputs(GeglGraph *self)
{
  gint i;
  gint num_inputs;

  GeglGraphSetupVisitor * graph_setup_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_SETUP_VISITOR, NULL); 

  graph_setup_visitor->graph = self;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_setup_visitor), 
                            self->root);

  if(self->graph_inputs)
    free_list(self, self->graph_inputs);
   
  self->graph_inputs = graph_setup_visitor->graph_inputs;
  g_object_unref(graph_setup_visitor);

  num_inputs = g_list_length(self->graph_inputs);
  for(i=0; i<num_inputs; i++)
    {
      GeglGraphInput *graph_input = g_list_nth_data(self->graph_inputs, i);
      GeglData *data = NULL;

      if(GEGL_IS_OP(graph_input->node))
          data = gegl_op_get_nth_input_data(GEGL_OP(graph_input->node), 
                                          graph_input->node_input);
      gegl_op_append_input_data(GEGL_OP(self), data);
    }
}

static void
reset_outputs(GeglGraph *self)
{
  gint i;
  gint num_outputs = gegl_node_get_num_outputs(self->root);

  for(i=0; i < num_outputs; i++)
    {
      GeglData *data = NULL;
      if(GEGL_IS_OP(self->root))
        data = gegl_op_get_nth_output_data(GEGL_OP(self->root), i);
      gegl_op_append_output_data(GEGL_OP(self), data);
    }
}

/**
 * gegl_graph_find_source:
 * @self: a #GeglGraph.
 * @node: a #GeglNode we want the source for.
 * @index: which input of the #GeglNode want source for. 
 *
 * Find a source for the graph that is used as the passed node's input. 
 *
 * Returns: a #GeglNode.
 **/
GeglNode *
gegl_graph_find_source(GeglGraph *self,
                       GeglNode *node,
                       gint index)
{
  GeglNode * source;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  source = gegl_node_get_source(node, index);

  /* If the source is null, try to get it from the graph */
  if(!source)
    {
      GeglGraphInput *graph_input = 
        gegl_graph_lookup_input(self, node, index); 

      if(graph_input)
        source = gegl_node_get_source(GEGL_NODE(self), graph_input->graph_input);
    }

  return source;
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
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_graph(visitor, GEGL_GRAPH(node));
}
