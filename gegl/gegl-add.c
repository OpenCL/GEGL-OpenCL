#include "gegl-add.h"
#include "gegl-add-impl.h"
#include "gegl-utils.h"

static void class_init (GeglAddClass * klass);
static void init (GeglAdd * self, GeglAddClass * klass);
static gpointer parent_class = NULL;

GType
gegl_add_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglAddClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglAdd),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglAdd", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglAddClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
  return;
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  self_op->op_impl = g_object_new(GEGL_TYPE_ADD_IMPL, NULL);

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
