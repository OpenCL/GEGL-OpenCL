#include "gil-mock-node.h"

enum
{
  PROP_0, 
  PROP_NUM_CHILDREN,
  PROP_CHILD0,
  PROP_CHILD1,
  PROP_LAST 
};

static void class_init (GilMockNodeClass * klass);
static void init(GilMockNode *self, GilMockNodeClass *klass);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_mock_node_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilMockNodeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilMockNode),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_NODE, "GilMockNode", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GilMockNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_NUM_CHILDREN,
                                   g_param_spec_int ("num_children",
                                                      "NumChildren",
                                                      "Number of children for GilMockNode",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHILD0,
                                   g_param_spec_object ("child0",
                                                        "Child0",
                                                        "Child 0 of this node",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHILD1,
                                   g_param_spec_object ("child1",
                                                        "Child1",
                                                        "Child 1 of this node",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GilMockNode * self, 
      GilMockNodeClass * klass)
{
  return;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_CHILDREN:
      {
        gil_node_set_num_children(node, g_value_get_int(value));  
      }
      break;
    case PROP_CHILD0:
      {
        gil_node_set_nth_child(node, (GilNode*)g_value_get_object(value), 0);  
      }
      break;
    case PROP_CHILD1:
      {
        gil_node_set_nth_child(node, (GilNode*)g_value_get_object(value), 1);  
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
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_CHILDREN:
      {
        g_value_set_int(value, gil_node_get_num_children(node));  
      }
      break;
    case PROP_CHILD0:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 0));  
      }
      break;
    case PROP_CHILD1:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 1));  
      }
      break;
    default:
      break;
  }
}
