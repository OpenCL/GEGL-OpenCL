#include "gil-binary-op.h"

enum
{
  PROP_0, 
  PROP_OP,
  PROP_LEFT_OPERAND,
  PROP_RIGHT_OPERAND,
  PROP_LAST 
};

static void class_init (GilBinaryOpClass * klass);
static void init(GilBinaryOp *self, GilBinaryOpClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_binary_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilBinaryOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilBinaryOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_EXPRESSION, 
                                     "GilBinaryOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GilBinaryOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OP,
                                   g_param_spec_int ("op",
                                                     "Op",
                                                     "Op type",
                                                      GIL_BINARY_OP_NONE + 1,
                                                      GIL_BINARY_OP_LAST - 1,
                                                      GIL_PLUS,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LEFT_OPERAND,
                                   g_param_spec_object ("left_operand",
                                                        "LeftOperand",
                                                        "Left operand of this op",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RIGHT_OPERAND,
                                   g_param_spec_object ("right_operand",
                                                        "RightOperand",
                                                        "Right operand of this op",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));
  return;
}

static void 
init (GilBinaryOp * self, 
      GilBinaryOpClass * klass)
{
  GilNode * node = GIL_NODE(self);
  self->op = GIL_BINARY_OP_NONE;
  gil_node_set_num_children(node, 2);
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
  GilBinaryOp *binary_op = GIL_BINARY_OP (gobject);
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_OP:
      {
        gil_binary_op_set_op(binary_op, g_value_get_int(value));  
      }
      break;
    case PROP_LEFT_OPERAND:
      {
        gil_node_set_nth_child(node, (GilNode*)g_value_get_object(value), 0);  
      }
      break;
    case PROP_RIGHT_OPERAND:
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
  GilBinaryOp *binary_op = GIL_BINARY_OP (gobject);
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_OP:
      {
        g_value_set_int(value, gil_binary_op_get_op(binary_op));  
      }
      break;
    case PROP_LEFT_OPERAND:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 0));  
      }
      break;
    case PROP_RIGHT_OPERAND:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 1));  
      }
      break;
    default:
      break;
  }
}

GilBinaryOpType
gil_binary_op_get_op (GilBinaryOp * self)
{
  g_return_val_if_fail (self, GIL_BINARY_OP_NONE);
  g_return_val_if_fail (GIL_IS_BINARY_OP (self), GIL_BINARY_OP_NONE);

  return self->op;
}

void 
gil_binary_op_set_op (GilBinaryOp * self, 
                      GilBinaryOpType op)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_BINARY_OP (self));

  self->op = op;
}
