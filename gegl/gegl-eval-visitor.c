#include "gegl-eval-visitor.h"
#include "gegl-filter.h"
#include "gegl-data.h"
#include "gegl-dump-visitor.h"
#include "gegl-graph.h"
#include "gegl-value-types.h"
#include "gegl-image-op.h"
#include "gegl-color-model.h"
#include "gegl-image.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglEvalVisitorClass * klass);
static void init (GeglEvalVisitor * self, GeglEvalVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_eval_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglEvalVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglEvalVisitor * self, 
      GeglEvalVisitorClass * klass)
{
}

static void
finalize(GObject *gobject)
{  
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GList * data_outputs;
  GList * data_inputs;

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);


  /* Construct and validate the data_inputs of op */
  data_inputs = gegl_visitor_collect_data_inputs(visitor, GEGL_NODE(filter));
  gegl_filter_validate_inputs(filter, data_inputs);

  /* Validate the data_outputs of op*/
  data_outputs = gegl_op_get_data_outputs(GEGL_OP(filter));
  gegl_filter_validate_outputs(filter, data_outputs);

  gegl_filter_evaluate(filter, data_outputs, data_inputs);

  g_list_free(data_inputs);
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  GeglGraph *prev_graph = visitor->graph;
  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  visitor->graph = graph;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(visitor), 
                            GEGL_NODE(graph->root));
  visitor->graph = prev_graph; 
}
