#include "gegl-graph-setup-visitor.h"
#include "gegl-node.h"
#include "gegl-filter.h"
#include "gegl-graph.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

static void class_init (GeglGraphSetupVisitorClass * klass);
static void init (GeglGraphSetupVisitor * self, GeglGraphSetupVisitorClass * klass);

static void visit_node (GeglVisitor *visitor, GeglNode * node);
static void visit_op (GeglVisitor *visitor, GeglOp * op);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_graph_setup_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglGraphSetupVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglGraphSetupVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglGraphSetupVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglGraphSetupVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
  visitor_class->visit_op = visit_op;
  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglGraphSetupVisitor * self, 
      GeglGraphSetupVisitorClass * klass)
{
  self->graph_inputs = NULL;
  self->graph = NULL;
}

static void      
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  GeglGraphSetupVisitor *self = GEGL_GRAPH_SETUP_VISITOR(visitor); 

  GEGL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);

  /* Check to see if this node is an graph input */
  {
    gint num_inputs = gegl_node_get_num_inputs(node); 
    gint i;

    for(i = 0; i < num_inputs; i++)
      {
        GeglNode * input = gegl_node_get_nth_input(node, i);

        if(!input) 
          {
            GeglGraphInput * graph_input = g_new(GeglGraphInput, 1);

            graph_input->node = node;
            graph_input->node_input_index = i;

            graph_input->graph = self->graph; 
            graph_input->graph_input_index = g_list_length(self->graph_inputs);

            self->graph_inputs = g_list_append(self->graph_inputs, graph_input);
          }
      }
  }
}

static void      
visit_op(GeglVisitor * visitor,
         GeglOp *op)
{
  visit_node(visitor, GEGL_NODE(op));
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  visit_node(visitor, GEGL_NODE(filter));
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  visit_node(visitor, GEGL_NODE(graph));
}
