#include "gegl-filter-inputs-visitor.h"
#include "gegl-op.h"
#include "gegl-filter.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglFilterInputsVisitorClass * klass);
static void init (GeglFilterInputsVisitor * self, GeglFilterInputsVisitorClass * klass);

static void visit_op (GeglVisitor *visitor, GeglOp * op);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_filter_inputs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterInputsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilterInputsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglFilterInputsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFilterInputsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_op = visit_op;
  visitor_class->visit_filter = visit_filter;
}

static void 
init (GeglFilterInputsVisitor * self, 
      GeglFilterInputsVisitorClass * klass)
{
  self->filter_inputs = NULL;
}

static void      
visit_op(GeglVisitor * visitor,
         GeglOp *op)
{
  GeglFilterInputsVisitor *self = GEGL_FILTER_INPUTS_VISITOR(visitor); 

  GEGL_VISITOR_CLASS(parent_class)->visit_op(visitor, op);

  LOG_DEBUG("visit_op", "visiting op %s %x", G_OBJECT_TYPE_NAME(op), (guint)op); 
  {
    gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(op)); 
    gint i;

    for(i = 0; i < num_inputs; i++)
      {
        GeglNode * input = gegl_node_get_nth_input(GEGL_NODE(op), i);

        if(!input) 
          {
            GeglFilterInput * filter_input = g_new(GeglFilterInput, 1);
            filter_input->input = GEGL_NODE(op);
            filter_input->index = i;
            self->filter_inputs = g_list_append(self->filter_inputs, filter_input);
          }
      }
  }
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  visit_op(visitor, GEGL_OP(filter));
}
