#include "gegl-mult.h"
#include "gegl-mult-impl.h"
#include "gegl-utils.h"

static void class_init (GeglMultClass * klass);
static void init (GeglMult * self, GeglMultClass * klass);
static gpointer parent_class = NULL;

GType
gegl_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMultClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
  return;
}

static void 
init (GeglMult * self, 
      GeglMultClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  self_op->op_impl = g_object_new(GEGL_TYPE_MULT_IMPL, NULL);

  {
    GeglNode * self_node = GEGL_NODE(self); 
    GList * inputs = g_list_append(NULL, NULL);
    inputs = g_list_append(inputs, NULL);
    self_node->num_inputs = 2;
    gegl_node_set_inputs(self_node, inputs);
    g_list_free(inputs);
  }

  return;
}
