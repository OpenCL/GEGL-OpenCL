#include "gil-unary-op.h"

enum
{
  PROP_0, 
  PROP_OP,
  PROP_OPERAND,
  PROP_LAST 
};

static void class_init (GilUnaryOpClass * klass);
static void init(GilUnaryOp *self, GilUnaryOpClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_unary_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilUnaryOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilUnaryOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_EXPRESSION, 
                                     "GilUnaryOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GilUnaryOpClass * klass)
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
                                                      GIL_UNARY_OP_NONE,
                                                      GIL_UNARY_OP_LAST - 1,
                                                      GIL_UNARY_OP_NONE,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_OPERAND,
                                   g_param_spec_object ("operand",
                                                        "Operand",
                                                        "Operand of this op",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GilUnaryOp * self, 
      GilUnaryOpClass * klass)
{
  GilNode * node = GIL_NODE(self);
  gil_node_set_num_children(node, 1);
  self->op = GIL_UNARY_OP_NONE;
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
  GilUnaryOp *unary_op = GIL_UNARY_OP (gobject);
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_OP:
      {
        gil_unary_op_set_op(unary_op, g_value_get_int(value));  
      }
      break;
    case PROP_OPERAND:
      {
        gil_node_set_nth_child(node, (GilNode*)g_value_get_object(value), 0);  
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
  GilUnaryOp *unary_op = GIL_UNARY_OP (gobject);
  GilNode *node = GIL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_OP:
      {
        g_value_set_int(value, gil_unary_op_get_op(unary_op));  
      }
      break;
    case PROP_OPERAND:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 0));  
      }
      break;
    default:
      break;
  }
}

GilUnaryOpType
gil_unary_op_get_op (GilUnaryOp * self)
{
  g_return_val_if_fail (self, GIL_UNARY_OP_NONE);
  g_return_val_if_fail (GIL_IS_UNARY_OP (self), GIL_UNARY_OP_NONE);

  return self->op;
}

void 
gil_unary_op_set_op (GilUnaryOp * self, 
                     GilUnaryOpType op)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_UNARY_OP (self));

  self->op = op;
}

