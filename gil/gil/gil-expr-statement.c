#include "gil-expr-statement.h"

enum
{
  PROP_0, 
  PROP_LEFT_EXPR,
  PROP_RIGHT_EXPR,
  PROP_LAST 
};

static void class_init (GilExprStatementClass * klass);
static void init(GilExprStatement *self, GilExprStatementClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_expr_statement_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilExprStatementClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilExprStatement),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_STATEMENT, 
                                     "GilExprStatement", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GilExprStatementClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_LEFT_EXPR,
                                   g_param_spec_object ("left_expr",
                                                        "LeftExpr",
                                                        "Left hand side of expression statement",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RIGHT_EXPR,
                                   g_param_spec_object ("right_expr",
                                                        "RightExpr",
                                                        "Right hand side of expression statement",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));
  return;
}

static void 
init (GilExprStatement * self, 
      GilExprStatementClass * klass)
{
  GilNode * node = GIL_NODE(self);
  gil_node_set_num_children(node,2);
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
  GilNode *node = GIL_NODE(gobject);

  switch (prop_id)
  {
    case PROP_LEFT_EXPR:
      {
        gil_node_set_nth_child(node, (GilNode*)g_value_get_object(value), 0);  
      }
      break;
    case PROP_RIGHT_EXPR:
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
    case PROP_LEFT_EXPR:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 0));  
      }
      break;
    case PROP_RIGHT_EXPR:
      {
        g_value_set_object(value, (GObject*)gil_node_get_nth_child(node, 1));  
      }
      break;
    default:
      break;
  }
}
