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

static void validate_have_rect(GeglData *input_data, GeglData *collected_input_data);
static void validate_color_model(GeglData *input_data, GeglData *collected_input_data);
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
validate_have_rect(GeglData *input_data, 
                   GeglData *collected_input_data)
{
  if(GEGL_IS_IMAGE_DATA(input_data))
    {
      GeglRect have_rect; 

      g_return_if_fail(GEGL_IS_IMAGE_DATA(collected_input_data));

      gegl_image_data_get_rect(GEGL_IMAGE_DATA(collected_input_data), &have_rect);
      gegl_image_data_set_rect(GEGL_IMAGE_DATA(input_data), &have_rect);
    }
}

static void
validate_color_model(GeglData *input_data, 
                     GeglData *collected_input_data)
{
  if(GEGL_IS_COLOR_DATA(input_data))
    {
      if(collected_input_data) 
        {
           g_return_if_fail(GEGL_IS_COLOR_DATA(collected_input_data));

           GeglColorModel *color_model = 
             gegl_color_data_get_color_model(GEGL_COLOR_DATA(collected_input_data));
           gegl_color_data_set_color_model(GEGL_COLOR_DATA(input_data), color_model);
        }
    }
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
      /* Copy the have rect from collected input data to input data */ 
      GList *collected_input_data_list = 
        gegl_visitor_collect_input_data_list(visitor, GEGL_NODE(filter));

      gegl_op_validate_input_data(GEGL_OP(filter), 
                                  collected_input_data_list, 
                                  &validate_have_rect);
    
      g_list_free(collected_input_data_list);

      /* Now compute the have rect for the output data */
      gegl_image_op_compute_have_rect(GEGL_IMAGE_OP(filter));
    }

  LOG_DEBUG("visit_filter", 
            "computing derived color_model for %s %p", 
            G_OBJECT_TYPE_NAME(filter), filter);

  if(GEGL_IS_IMAGE_OP(filter))
    {
      /* Copy the color model from collected input data to input data */ 
      GList *collected_input_data_list = 
        gegl_visitor_collect_input_data_list(visitor, GEGL_NODE(filter));

      gegl_op_validate_input_data(GEGL_OP(filter), 
                                  collected_input_data_list, 
                                  &validate_color_model);
    
      g_list_free(collected_input_data_list);

      /* Now compute the color model for the output data */
      gegl_image_op_compute_color_model(GEGL_IMAGE_OP(filter));
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
