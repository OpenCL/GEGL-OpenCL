#include "gegl-eval-dfs-visitor.h"
#include "gegl-filter.h"
#include "gegl-graph.h"
#include "gegl-image-op.h"
#include "gegl-color-model.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

static void class_init (GeglEvalDfsVisitorClass * klass);
static void init (GeglEvalDfsVisitor * self, GeglEvalDfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_eval_dfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalDfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalDfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglEvalDfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalDfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglEvalDfsVisitor * self, 
      GeglEvalDfsVisitorClass * klass)
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
  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  LOG_DEBUG("visit_filter", 
            "computing have rect for %s %p", 
            G_OBJECT_TYPE_NAME(filter),filter);


  if(GEGL_IS_IMAGE_OP(filter))
  {
    GList * data_inputs = gegl_visitor_collect_data_inputs(visitor, GEGL_NODE(filter));
    gegl_image_op_evaluate_have_rect(GEGL_IMAGE_OP(filter), data_inputs);
    g_list_free(data_inputs);
  }

  LOG_DEBUG("visit_filter", 
            "computing derived color_model for %s %p", 
            G_OBJECT_TYPE_NAME(filter), filter);

  if(GEGL_IS_IMAGE_OP(filter))
  {
    GList * data_inputs = gegl_visitor_collect_data_inputs(visitor, GEGL_NODE(filter));
    gegl_image_op_evaluate_color_model(GEGL_IMAGE_OP(filter), data_inputs);
    g_list_free(data_inputs);
  }
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  GeglGraph *prev_graph = visitor->graph;

  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  /* Set the current one. */
  visitor->graph = graph;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(visitor), 
                            GEGL_NODE(graph->root));

  /* Restore the old graph. */
  visitor->graph = prev_graph; 
}
