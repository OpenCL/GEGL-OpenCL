#include "gegl-premult.h"
#include "gegl-premult-impl.h"
#include "gegl-image-mgr.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include <stdio.h>

static void class_init (GeglPremultClass * klass);
static void init (GeglPremult * self, GeglPremultClass * klass);

static gpointer parent_class = NULL;

GType
gegl_premult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPremultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPremult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglPremult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
init (GeglPremult * self, 
      GeglPremultClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  self_op->op_impl = g_object_new(GEGL_TYPE_PREMULT_IMPL, NULL);

  {
    GeglNode * self_node = GEGL_NODE(self); 
    GList * inputs = g_list_append(NULL, NULL);
    self_node->num_inputs = 1;
    gegl_node_set_inputs(self_node, inputs);
    g_list_free(inputs);
  }

  return;
}

static void 
class_init (GeglPremultClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);

  return;
}
