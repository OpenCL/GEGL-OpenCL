#include "gegl-print.h"
#include "gegl-print-impl.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglPrintClass * klass);
static void init (GeglPrint * self, GeglPrintClass * klass);
static gpointer parent_class = NULL;

GType
gegl_print_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPrintClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPrint),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_STAT_OP , 
                                     "GeglPrint", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPrintClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);

  return;
}

static void 
init (GeglPrint * self, 
      GeglPrintClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  self_op->op_impl = g_object_new(GEGL_TYPE_PRINT_IMPL, NULL);

  {
    GeglNode * self_node = GEGL_NODE(self); 
    GList * inputs = g_list_append(NULL, NULL);
    self_node->num_inputs = 1;
    gegl_node_set_inputs(self_node, inputs);
    g_list_free(inputs);
  }

  return;
}
