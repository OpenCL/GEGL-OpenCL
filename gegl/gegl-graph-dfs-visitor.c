#include "gegl-graph-dfs-visitor.h"
#include "gegl-filter.h"
#include "gegl-graph.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-color-model.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-tile.h"
#include "gegl-attributes.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglGraphDfsVisitorClass * klass);
static void init (GeglGraphDfsVisitor * self, GeglGraphDfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static void compute_have_rect(GeglGraphDfsVisitor * self, GeglFilter * filter);
static void compute_derived_color_model(GeglGraphDfsVisitor * self, GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_graph_dfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglGraphDfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglGraphDfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglGraphDfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglGraphDfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglGraphDfsVisitor * self, 
      GeglGraphDfsVisitorClass * klass)
{
  self->graph = NULL;
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
  GeglGraphDfsVisitor *self = GEGL_GRAPH_DFS_VISITOR(visitor); 
  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  LOG_DEBUG("visit_filter", 
            "computing have rect for %s %p", 
            G_OBJECT_TYPE_NAME(filter),filter);

  compute_have_rect(self, filter); 

  LOG_DEBUG("visit_filter", 
            "computing derived color_model for %s %p", 
            G_OBJECT_TYPE_NAME(filter), filter);

  compute_derived_color_model(self, filter);
}

static void
compute_have_rect(GeglGraphDfsVisitor * self,
                  GeglFilter * filter)
{
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(filter)); 
  gint i;
  GeglRect have_rect;
  GeglRect result_rect;
  GeglRect need_rect;
  GList * input_have_rects = NULL;
  GList * input_attributes_list = 
    gegl_graph_get_input_attributes(self->graph, GEGL_NODE(filter));

  GeglAttributes *attributes = 
    gegl_op_get_nth_attributes(GEGL_OP(filter), 0); 

  for(i = 0; i < num_inputs; i++)
    {
      GeglAttributes *input_attributes = 
        (GeglAttributes*)g_list_nth_data(input_attributes_list, i); 

      input_have_rects = g_list_append(input_have_rects, &input_attributes->rect); 
    }

  /* Pass these and let node compute have rect */
  gegl_filter_compute_have_rect(filter, &have_rect, input_have_rects); 

  /* Intersect the have and the need rect to get the result rect */
  gegl_attributes_get_rect(attributes, &need_rect);
  gegl_rect_intersect(&result_rect, &need_rect, &have_rect);

  /* Store this result rect */
  gegl_attributes_set_rect(attributes, &result_rect);

  g_list_free(input_have_rects);
  g_list_free(input_attributes_list);
}

static void
compute_derived_color_model(GeglGraphDfsVisitor * self,
                            GeglFilter * filter)
{
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(filter)); 
  gint i;
  GList * input_color_models = NULL;
  GeglColorModel *color_model;

  GList * input_attributes_list = 
    gegl_graph_get_input_attributes(self->graph, GEGL_NODE(filter));

  GeglAttributes *attributes = 
    gegl_op_get_nth_attributes(GEGL_OP(filter), 0); 

  /* Collect the color models of the inputs. */
  for(i = 0; i < num_inputs; i++)
    {
      GeglAttributes *input_attributes = 
        (GeglAttributes*)g_list_nth_data(input_attributes_list, i); 

      GeglColorModel *input_color_model = 
        gegl_attributes_get_color_model(input_attributes); 

      input_color_models = g_list_append(input_color_models, input_color_model); 
    }

  color_model = gegl_filter_compute_derived_color_model(filter, input_color_models); 
  gegl_attributes_set_color_model(attributes, color_model);

  g_list_free(input_color_models);
  g_list_free(input_attributes_list);
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  GeglGraphDfsVisitor *self = GEGL_GRAPH_DFS_VISITOR(visitor); 
  GeglGraph *prev_graph = self->graph;

  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  /* Set the current one. */
  self->graph = graph;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(visitor), 
                            GEGL_NODE(graph->root));

  /* Restore the old graph. */
  self->graph = prev_graph; 
}
