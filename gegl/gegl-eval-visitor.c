#include "gegl-eval-visitor.h"
#include "gegl-filter.h"
#include "gegl-attributes.h"
#include "gegl-dump-visitor.h"
#include "gegl-graph.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-color-model.h"
#include "gegl-image-data.h"
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
  GList * input_attributes;
  GeglAttributes * attributes;

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);
  
  input_attributes = gegl_visitor_get_input_attributes(visitor, GEGL_NODE(filter));
  gegl_filter_validate_inputs(filter, input_attributes);

  attributes = gegl_op_get_attributes(GEGL_OP(filter));
  gegl_filter_validate_outputs(filter, attributes);

  gegl_filter_evaluate(filter, attributes, input_attributes);

  g_list_free(input_attributes);
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
