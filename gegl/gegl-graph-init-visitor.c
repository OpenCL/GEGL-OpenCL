#include "gegl-graph-init-visitor.h"
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

static void class_init (GeglGraphInitVisitorClass * klass);
static void init (GeglGraphInitVisitor * self, GeglGraphInitVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_graph_init_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglGraphInitVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglGraphInitVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglGraphInitVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglGraphInitVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglGraphInitVisitor * self, 
      GeglGraphInitVisitorClass * klass)
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
  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);
}

static void      
visit_graph(GeglVisitor * visitor,
            GeglGraph *graph)
{
  GeglGraphInitVisitor *self = GEGL_GRAPH_INIT_VISITOR(visitor); 
  GeglGraph *prev_graph = self->graph;

  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  self->graph = graph;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(visitor), 
                            GEGL_NODE(graph->root));
  self->graph = prev_graph; 

  /* If its the top level one, 
     then we must copy roi and 
     image to attributes of graph. */
  if(!self->graph)
  {
    GeglAttributes * attr = gegl_op_get_nth_attributes(GEGL_OP(graph),0);

    gegl_attributes_set_rect(attr, &graph->roi);

    if(graph->image)
      g_value_set_object(attr->value, GEGL_IMAGE(graph->image)->tile);  
  }
}
