#include "gegl-graph.h"
#include "gegl-node.h"
#include "gegl-image.h"
#include "gegl-visitor.h"
#include "gegl-attributes.h"
#include "gegl-color-model.h"
#include "gegl-graph-setup-visitor.h"
#include "gegl-graph-bfs-visitor.h"
#include "gegl-graph-dfs-visitor.h"
#include "gegl-graph-evaluate-visitor.h"
#include "gegl-graph-init-visitor.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_ROOT, 
  PROP_IMAGE, 
  PROP_ROI, 
  PROP_LAST 
};

static void class_init (GeglGraphClass * klass);
static void init (GeglGraph * self, GeglGraphClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void execute (GeglGraph * self);
static void allocate_attributes(GeglOp *op);

static void accept (GeglNode * node, GeglVisitor * visitor);
static void free_list(GeglGraph *self, GList *list);

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
  op_class->allocate_attributes = allocate_attributes;

  klass->execute = execute;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this graph",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        "image",
                                                        "A destination image for result",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ROI,
                                   g_param_spec_pointer ("roi",
                                                        "roi",
                                                        "Region of interest for graph",
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglGraph * self, 
      GeglGraphClass * klass)
{
  self->image = NULL;
  self->root = NULL;

  gegl_rect_set(&self->roi,0,0,GEGL_DEFAULT_WIDTH,GEGL_DEFAULT_HEIGHT);

  return;
}

static void
finalize(GObject *gobject)
{  
  GeglGraph *self = GEGL_GRAPH(gobject);
  GeglOp *op = GEGL_OP(gobject);

  free_list(self, self->graph_inputs);
  free_list(self, self->graph_outputs);

  /* Set the attributes list to NULL */
  op->attributes = NULL;

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
    case PROP_IMAGE:
        gegl_graph_set_image(graph, (GeglSampledImage*)g_value_get_object(value));  
      break;
    case PROP_ROI:
        gegl_graph_set_roi (graph, (GeglRect*)g_value_get_pointer (value));
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
    case PROP_IMAGE:
      g_value_set_object(value, (GObject*)gegl_graph_get_image(graph));  
      break;
    case PROP_ROI:
      gegl_graph_get_roi (graph, (GeglRect*)g_value_get_pointer (value));
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

  LOG_DEBUG("set_root", "setting the root here");
  self->root = root;

  {
    GeglGraphSetupVisitor * graph_setup_visitor = 
        g_object_new(GEGL_TYPE_GRAPH_SETUP_VISITOR, NULL); 

    graph_setup_visitor->graph = self;
    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_setup_visitor), 
                              self->root);

    self->graph_inputs = graph_setup_visitor->graph_inputs;
    g_object_unref(graph_setup_visitor);

    gegl_node_set_num_inputs(GEGL_NODE(self), 
                             g_list_length(self->graph_inputs));

    gegl_node_set_num_outputs(GEGL_NODE(self), 1);

    {
      GeglGraphOutput * graph_output = g_new(GeglGraphOutput, 1);
      graph_output->node = root;
      graph_output->node_output_index = 0;
      graph_output->graph = self; 
      graph_output->graph_output_index = 0;
      self->graph_outputs = g_list_append(self->graph_outputs, graph_output); 
    }

    gegl_op_allocate_attributes(GEGL_OP(self));
  }
}

/**
 * gegl_graph_get_roi:
 * @self: a #GeglGraph.
 * @roi: a #GeglRect.
 *
 * Gets the roi of this graph.
 *
 **/
void
gegl_graph_get_roi(GeglGraph *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_GRAPH (self));

  gegl_rect_copy(roi, &self->roi);
}

/**
 * gegl_graph_set_roi:
 * @self: a #GeglGraph.
 * @roi: a #GeglRect.
 *
 * Sets the roi of this graph.
 *
 **/
void
gegl_graph_set_roi(GeglGraph *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_GRAPH (self));

  if(roi)
    gegl_rect_copy(&self->roi, roi);
  else
    {
      if(self->image && GEGL_IS_SAMPLED_IMAGE(self->image))
        {
          gint width = gegl_sampled_image_get_width(self->image);
          gint height = gegl_sampled_image_get_height(self->image);
          gegl_rect_set(&self->roi,0,0,width,height);
        }
      else
        {
            gegl_rect_set(&self->roi,0,0,720,486);
        }
    }
}

/**
 * gegl_graph_get_image:
 * @self: a #GeglGraph.
 *
 * Gets the image of this graph.
 *
 * Returns: a #GeglSampledImage
 **/
GeglSampledImage* 
gegl_graph_get_image (GeglGraph * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return self->image;
}

/**
 * gegl_graph_set_image:
 * @self: a #GeglGraph.
 * @image: a #GeglSampledImage.
 *
 * Sets the roi of this graph.
 *
 **/
void 
gegl_graph_set_image (GeglGraph * self,
                      GeglSampledImage *image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_GRAPH (self));

  self->image = image;
}

/**
 * gegl_graph_get_input_attributes:
 * @self: a #GeglGraph.
 * @node: a #GeglNode.
 *
 * Get a list of input attributes for this node. Free the list when done.
 *
 * Returns: a GList of the input attributes for the node. 
 **/
GList * 
gegl_graph_get_input_attributes(GeglGraph *self,
                                GeglNode *node)
{
  GList * input_attributes_list = NULL;
  gint i;
  gint num_inputs = gegl_node_get_num_inputs(node);

  for(i = 0 ; i < num_inputs; i++) 
    {
      GeglConnection *connection = 
        gegl_graph_find_input_connection(self, node, i);

      GeglNode * input = connection->input;
      gint output_index = connection->output_index;
      GeglAttributes *input_attributes = 
        gegl_op_get_nth_attributes(GEGL_OP(input), output_index); 

      input_attributes_list = 
        g_list_append(input_attributes_list, input_attributes);
    }

  return input_attributes_list;
}


/**
 * gegl_graph_find_input_connection:
 * @self: a #GeglGraph.
 * @node: a #GeglNode.
 * @index: which input index of the node.
 *
 * Find the input connection for this node.
 *
 * Returns: a GeglConnection for the node. 
 **/
GeglConnection *
gegl_graph_find_input_connection(GeglGraph *self,
                                 GeglNode *node,
                                 gint index)
{
  GeglConnection * connection = 
    gegl_node_get_nth_input_connection(node, index);

  /* If the input is null, must get it from the graph */
  if(!connection->input)
    {
      GeglGraphInput *graph_input = 
        gegl_graph_lookup_input(self, node, index); 

      /* connection from graph to input index */
      connection = 
        gegl_node_get_nth_input_connection(GEGL_NODE(self), 
                                           graph_input->graph_input_index); 
    }

  return connection;
}

/**
 * gegl_graph_lookup_output:
 * @self: a #GeglGraph.
 * @graph_output_index: output index of graph. 
 *
 * Lookup the #GeglGraphOutput for this output index. 
 *
 * Returns: a #GeglGraphOutput.
 **/
GeglGraphOutput *
gegl_graph_lookup_output(GeglGraph *self,
                         gint graph_output_index)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return (GeglGraphOutput*)g_list_nth_data(self->graph_outputs, graph_output_index);
}

/**
 * gegl_graph_lookup_input:
 * @self: a #GeglGraph.
 * @node: a #GeglNode.
 * @node_input_index: input index of node. 
 *
 * Look for a #GeglGraphInput for this node. 
 *
 * Returns: a #GeglGraphInput or NULL if not found.
 **/
GeglGraphInput *
gegl_graph_lookup_input(GeglGraph *self,
                        GeglNode *node,
                        gint node_input_index)
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
            graph_input->node_input_index == node_input_index) 
             return graph_input;
      }
  }

  return NULL; 
}

/**
 * gegl_graph_execute:
 * @self: a #GeglGraph.
 *
 * Executes this graph.
 *
 **/
void 
gegl_graph_execute(GeglGraph *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_GRAPH (self));

  {
    GeglGraphClass *klass = GEGL_GRAPH_GET_CLASS(self);

    if(klass->execute)
        (*klass->execute)(self);
  }
}

static void      
execute (GeglGraph * self) 
{
  GeglOp * root = GEGL_OP(self->root); 
  LOG_DEBUG("execute", 
            "begin graph init for %s %p", 
            G_OBJECT_TYPE_NAME(root), 
            root);
  {
    GeglGraphInitVisitor * graph_init_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_INIT_VISITOR, NULL); 

    graph_init_visitor->graph = NULL;
    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_init_visitor),
                              GEGL_NODE(self));

    g_object_unref(graph_init_visitor);
  }

  LOG_DEBUG("execute", 
            "begin bfs for %s %p", 
            G_OBJECT_TYPE_NAME(root), 
            root);
  {
    GeglGraphBfsVisitor * graph_bfs_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_BFS_VISITOR, NULL); 

    graph_bfs_visitor->graph = NULL;
    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(graph_bfs_visitor),
                              GEGL_NODE(self));

    g_object_unref(graph_bfs_visitor);
  }

  LOG_DEBUG("execute", 
            "begin dfs for %s %p", 
            G_OBJECT_TYPE_NAME(root), 
            root);
  {
    GeglGraphDfsVisitor * graph_dfs_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_DFS_VISITOR, NULL); 

    graph_dfs_visitor->graph = NULL;
    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_dfs_visitor),
                              GEGL_NODE(self));

    g_object_unref(graph_dfs_visitor);
  }

  LOG_DEBUG("execute", 
            "begin evaluate dfs for %s %p", 
            G_OBJECT_TYPE_NAME(root), 
            root);
  {
    GeglGraphEvaluateVisitor * graph_evaluate_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_EVALUATE_VISITOR, NULL); 

    graph_evaluate_visitor->graph = NULL;
    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_evaluate_visitor),
                              GEGL_NODE(self));

    g_object_unref(graph_evaluate_visitor);
  }
}

static void
allocate_attributes(GeglOp *op)
{
  gint i;
  GeglGraph *self = GEGL_GRAPH(op);
  gint num_output_values = 
    gegl_node_get_num_outputs(GEGL_NODE(op));

  g_return_if_fail (op != NULL);
  g_return_if_fail (GEGL_IS_GRAPH(op));
  g_return_if_fail (num_output_values >= 0);

   /* 
     Now figure out how to get the output attributes from 
     the outputs of the graph 
   */

  LOG_DEBUG("allocate_attributes", "setting attributes to whatever");
  for(i = 0; i < num_output_values; i++)
    {
      GeglGraphOutput *graph_output = 
        (GeglGraphOutput*)g_list_nth_data(self->graph_outputs, i);

      if(GEGL_IS_OP(graph_output->node))
      op->attributes[i] = 
        gegl_op_get_nth_attributes(GEGL_OP(graph_output->node), 
                                   graph_output->node_output_index);
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
