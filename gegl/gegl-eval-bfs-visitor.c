#include "gegl-eval-bfs-visitor.h"
#include "gegl-filter.h"
#include "gegl-image-op.h"
#include "gegl-image-data.h"
#include "gegl-graph.h"

static void class_init (GeglEvalBfsVisitorClass * klass);
static void init (GeglEvalBfsVisitor * self, GeglEvalBfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static void validate_need_rect(GeglData *input_data, GeglData *collected_input_data);

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
        NULL
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
validate_need_rect(GeglData *input_data,
                   GeglData *collected_input_data)
{
  if(GEGL_IS_IMAGE_DATA(input_data))
    {
      GeglRect need_rect; 

      g_return_if_fail(GEGL_IS_IMAGE_DATA(collected_input_data));

      gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &need_rect);
      gegl_image_data_set_rect(GEGL_IMAGE_DATA(collected_input_data), &need_rect);
    }
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  gegl_log_debug("visit_filter", 
            "computing need rects for inputs of %s %p", 
            G_OBJECT_TYPE_NAME(filter),filter);

  if(GEGL_IS_IMAGE_OP(filter))
  {
    GArray *collected_data; 
    gegl_image_op_compute_need_rects(GEGL_IMAGE_OP(filter));

    collected_data = gegl_visitor_collect_input_data(visitor, GEGL_NODE(filter));

    gegl_op_validate_input_data_array(GEGL_OP(filter), 
                                      collected_data, 
                                      &validate_need_rect);
  
    g_array_free(collected_data, TRUE);
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
