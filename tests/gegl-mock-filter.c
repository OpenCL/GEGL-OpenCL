#include "gegl-mock-filter.h"

enum
{
  PROP_0, 
  PROP_NUM_INPUTS,
  PROP_NUM_OUTPUTS,
  PROP_INPUT0,
  PROP_INPUT1,
  PROP_LAST 
};

static void class_init        (GeglMockFilterClass * klass);
static void init (GeglMockFilter * self, GeglMockFilterClass * klass);
static void finalize (GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static gpointer parent_class = NULL;

GType
gegl_mock_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglMockFilter", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_NUM_INPUTS,
                                   g_param_spec_int ("num_inputs",
                                                      "NumInputs",
                                                      "Number of inputs for GeglMockFilter",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_OUTPUTS,
                                   g_param_spec_int ("num_outputs",
                                                      "NumOutputs",
                                                      "Number of outputs for GeglMockFilter",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INPUT0,
                                   g_param_spec_object ("input0",
                                                        "Input0",
                                                        "Input 0 of this node",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INPUT1,
                                   g_param_spec_object ("input1",
                                                        "Input1",
                                                        "Input 1 of this node",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_READWRITE));


}

static void 
init (GeglMockFilter * self, 
      GeglMockFilterClass * klass)
{
  GeglNode * node = GEGL_NODE(self);
  GeglOp *op = GEGL_OP(self);
  GeglAttributes *attributes;

  gegl_node_set_num_outputs(node, 1);
  gegl_node_set_num_inputs(node, 0);
  gegl_op_allocate_attributes(op);

  attributes = gegl_op_get_nth_attributes(op, 0);
  g_value_init(attributes->value, GEGL_TYPE_OBJECT);

  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_INPUTS:
      {
        gegl_node_set_num_inputs(node, g_value_get_int(value));  
      }
      break;
    case PROP_NUM_OUTPUTS:
      {
        gegl_node_set_num_outputs(node, g_value_get_int(value));  
      }
      break;
    case PROP_INPUT0:
      {
        gegl_node_set_nth_input(node, (GeglNode*)g_value_get_object(value), 0);  
      }
      break;
    case PROP_INPUT1:
      {
        gegl_node_set_nth_input(node, (GeglNode*)g_value_get_object(value), 1);  
      }
      break;
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_INPUTS:
      {
        g_value_set_int(value, gegl_node_get_num_inputs(node));  
      }
      break;
    case PROP_NUM_OUTPUTS:
      {
        g_value_set_int(value, gegl_node_get_num_outputs(node));  
      }
      break;
    case PROP_INPUT0:
      {
        g_value_set_object(value, (GObject*)gegl_node_get_nth_input(node, 0));  
      }
      break;
    case PROP_INPUT1:
      {
        g_value_set_object(value, (GObject*)gegl_node_get_nth_input(node, 1));  
      }
    default:
      break;
  }
}
