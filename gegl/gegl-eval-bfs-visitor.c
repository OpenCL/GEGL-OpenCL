#include "gegl-eval-bfs-visitor.h"
#include "gegl-filter.h"
#include "gegl-node.h"
#include "gegl-graph.h"
#include "gegl-attributes.h"
#include "gegl-value-types.h"

static void class_init (GeglEvalBfsVisitorClass * klass);
static void init (GeglEvalBfsVisitor * self, GeglEvalBfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static void compute_need_rects(GeglEvalBfsVisitor * self, GeglFilter * filter); 

static gpointer parent_class = NULL;

GType
gegl_eval_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalBfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_BFS_VISITOR, 
                                     "GeglEvalBfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalBfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglEvalBfsVisitor * self, 
      GeglEvalBfsVisitorClass * klass)
{
}

static void
finalize(GObject *gobject)
{  
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
compute_need_rects(GeglEvalBfsVisitor * self, 
                   GeglFilter * filter) 
{
  GeglVisitor *visitor = GEGL_VISITOR(self);
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(filter)); 
  gint i; 

  GList * input_attributes_list = 
    gegl_visitor_get_input_attributes(visitor, GEGL_NODE(filter));

  /* Warning: using the 0th output attributes only */
  GeglAttributes *attributes = 
    gegl_op_get_nth_attributes(GEGL_OP(filter), 0);

  for(i = 0 ; i < num_inputs; i++) 
    {
      GeglRect need_rect;
      GeglRect input_need_rect;

      GeglAttributes *input_attributes = 
        (GeglAttributes*)g_list_nth_data(input_attributes_list, i); 

      gegl_attributes_get_rect(attributes, &need_rect);
      gegl_attributes_get_rect(input_attributes, &input_need_rect);

      /* Compute the need rect of this input */
      gegl_filter_compute_need_rect(filter, &input_need_rect, &need_rect, i); 

      /* Store it in the inputs attributes */
      gegl_attributes_set_rect(input_attributes, &input_need_rect);
    }
}

static void      
visit_filter(GeglVisitor * visitor,
         GeglFilter *filter)
{
  GeglEvalBfsVisitor *self = GEGL_EVAL_BFS_VISITOR(visitor); 

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  LOG_DEBUG("visit_filter", 
            "computing need rects for inputs of %s %p", 
            G_OBJECT_TYPE_NAME(filter),filter);

  compute_need_rects(self, filter);
}


static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  /* Save the previous graph */
  GeglGraph *prev_graph = visitor->graph;

  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  /* Set the current one. */
  visitor->graph = graph;

  gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(visitor),
                            GEGL_NODE(graph->root));

  /* Restore the old graph. */
  visitor->graph = prev_graph; 
}
