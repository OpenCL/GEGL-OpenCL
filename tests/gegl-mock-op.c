#include "gegl-mock-op.h"

enum
{
  PROP_0, 
  PROP_INPUT_0,
  PROP_INPUT_1,
  PROP_INPUT_2,
  PROP_LAST 
};

static void class_init        (GeglMockOpClass * klass);
static void init (GeglMockOp * self, GeglMockOpClass * klass);
static void finalize (GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gpointer parent_class = NULL;

GType
gegl_mock_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OP, 
                                     "GeglMockOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_INPUT_0,
               g_param_spec_object ("input-0",
                                    "Input0",
                                    "The input 0",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_1,
               g_param_spec_object ("input-1",
                                    "Input1",
                                    "The input 1",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_2,
               g_param_spec_object ("input-2",
                                    "Input2",
                                    "The input 2",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

}

static void 
init (GeglMockOp * self, 
      GeglMockOpClass * klass)
{
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockOp *self = GEGL_MOCK_OP(gobject);
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
  GeglMockOp *self = GEGL_MOCK_OP(gobject);
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
