#include "gegl-eval-bfs-visitor.h"
#include "gegl-filter.h"
#include "gegl-image-op.h"
#include "gegl-graph.h"

static void class_init (GeglEvalBfsVisitorClass * klass);
static void init (GeglEvalBfsVisitor * self, GeglEvalBfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

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
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  LOG_DEBUG("visit_filter", 
            "computing need rects for inputs of %s %p", 
            G_OBJECT_TYPE_NAME(filter),filter);

  if(GEGL_IS_IMAGE_OP(filter))
  {
    GList * data_inputs = gegl_visitor_collect_data_inputs(visitor, GEGL_NODE(filter));
    gegl_image_op_evaluate_need_rects(GEGL_IMAGE_OP(filter), data_inputs);
    g_list_free(data_inputs);
  }
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
