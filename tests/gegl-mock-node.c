#include "gegl-mock-node.h"

enum
{
  PROP_0, 
  PROP_INPUT_0,
  PROP_INPUT_1,
  PROP_INPUT_2,
  PROP_LAST 
};

static void class_init (GeglMockNodeClass * klass);
static void init(GeglMockNode *self, GeglMockNodeClass *klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

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
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_NODE, "GeglMockNode", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglMockNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_INPUT_0,
               g_param_spec_object ("input-0",
                                    "Input0",
                                    "The input 0",
                                     GEGL_TYPE_NODE,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_1,
               g_param_spec_object ("input-1",
                                    "Input1",
                                    "The input 1",
                                     GEGL_TYPE_NODE,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_2,
               g_param_spec_object ("input-2",
                                    "Input2",
                                    "The input 2",
                                     GEGL_TYPE_NODE,
                                     G_PARAM_WRITABLE));
}

static void 
init (GeglMockNode * self, 
      GeglMockNodeClass * klass)
{
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockNode *self = GEGL_MOCK_NODE(gobject);
  switch (prop_id)
  {
    default:
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglMockNode *self = GEGL_MOCK_NODE(gobject);
  switch (prop_id)
  {
    case PROP_INPUT_0:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), input, 0);  
      }
      break;
    case PROP_INPUT_1:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), input, 1);  
      }
      break;
    case PROP_INPUT_2:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), input, 2);  
      }
      break;
    default:
      break;
  }
}
