#include "gegl-mock-node.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMockNodeClass * klass);
static void init(GeglMockNode *self, GeglMockNodeClass *klass);

static gpointer parent_class = NULL;

GType
gegl_mock_node_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockNodeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockNode),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NODE, "GeglMockNode", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglMockNodeClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglMockNode * self, 
      GeglMockNodeClass * klass)
{
  g_object_set(self, "num_inputs", 0, NULL);
  g_object_set(self, "num_outputs", 0, NULL);
}
