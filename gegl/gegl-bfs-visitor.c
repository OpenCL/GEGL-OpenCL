#include "gegl-bfs-visitor.h"
#include "gegl-op.h"
#include "gegl-filter.h"
#include "gegl-value-types.h"

static void class_init (GeglBfsVisitorClass * klass);
static void init (GeglBfsVisitor * self, GeglBfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_op (GeglVisitor *visitor, GeglOp * op);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static GList * get_input_values(GeglBfsVisitor * self, GeglOp *op);

static gpointer parent_class = NULL;

GType
gegl_bfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglBfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglBfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_op = visit_op;
  visitor_class->visit_filter = visit_filter;
}

static void 
init (GeglBfsVisitor * self, 
      GeglBfsVisitorClass * klass)
{
  self->input_values = NULL;
  self->filter = NULL;
}

static void
finalize(GObject *gobject)
{  
  GeglBfsVisitor * self = GEGL_BFS_VISITOR(gobject);

  if(self->input_values)
    g_list_free(self->input_values);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void      
visit_op(GeglVisitor * visitor,
         GeglOp *op)
{
  GeglBfsVisitor *self = GEGL_BFS_VISITOR(visitor); 
  GList * input_values = get_input_values(self, op);

  LOG_DEBUG("visit_op", 
            "computing need rects for inputs of %s %x", 
            G_OBJECT_TYPE_NAME(op),(guint)op);
  gegl_op_compute_need_rects(op, input_values);

  if(input_values)
    g_list_free(input_values);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GeglBfsVisitor *self = GEGL_BFS_VISITOR(visitor); 
  GValue *root_output_value = gegl_op_get_nth_output_value(filter->root, 0);
  GValue *filter_output_value = gegl_op_get_nth_output_value(GEGL_OP(filter), 0);

  LOG_DEBUG("visit_filter", 
             "setting root need rect from filter need rect");

  g_value_copy(filter_output_value, root_output_value);

  g_value_print_image_data(filter_output_value, "filter");
  g_value_print_image_data(root_output_value, "root");

  LOG_DEBUG("visit_filter", 
            "creating bfs visitor for filter %x",
             (guint)filter);

  {
    GeglBfsVisitor * bfs_visitor = g_object_new(GEGL_TYPE_BFS_VISITOR, NULL); 
    bfs_visitor->input_values = get_input_values(self, GEGL_OP(filter));
    bfs_visitor->filter = filter;
    gegl_node_traverse_breadth_first(GEGL_NODE(filter->root), 
                                     GEGL_VISITOR(bfs_visitor));
    g_object_unref(bfs_visitor);
  }
}

static GList *
get_input_values(GeglBfsVisitor * self,
                 GeglOp *op)
{
  GList * input_values;

  if(self->filter)
    input_values = gegl_filter_get_input_values(self->filter, op);
  else
    input_values = NULL;

  return input_values;
}
