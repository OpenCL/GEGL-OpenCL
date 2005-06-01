#include "gil-expression.h"

static void class_init (GilExpressionClass * klass);
static void init(GilExpression *self, GilExpressionClass *klass);
static void finalize(GObject * self_object);

static gpointer parent_class = NULL;

GType
gil_expression_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilExpressionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilExpression),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_NODE,
                                     "GilExpression",
                                     &typeInfo,
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void
class_init (GilExpressionClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  return;
}

static void
init (GilExpression * self,
      GilExpressionClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}
