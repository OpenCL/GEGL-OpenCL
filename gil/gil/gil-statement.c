#include "gil-statement.h"

enum
{
  PROP_0, 
  PROP_POSITION, 
  PROP_LAST 
};

static void class_init (GilStatementClass * klass);
static void init(GilStatement *self, GilStatementClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_statement_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilStatementClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilStatement),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_NODE, 
                                     "GilStatement", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GilStatementClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_POSITION,
                                   g_param_spec_int ("position",
                                                      "Position",
                                                      "Position in file",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  return;
}

static void 
init (GilStatement * self, 
      GilStatementClass * klass)
{
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
  GilStatement *statement = GIL_STATEMENT (gobject);

  switch (prop_id)
  {
    case PROP_POSITION:
      {
        gil_statement_set_position(statement, g_value_get_int(value));  
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
  GilStatement *statement = GIL_STATEMENT (gobject);

  switch (prop_id)
  {
    case PROP_POSITION:
      {
        g_value_set_int(value, gil_statement_get_position(statement));  
      }
      break;
    default:
      break;
  }
}

gint 
gil_statement_get_position (GilStatement * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GIL_IS_STATEMENT (self), -1);

  return self->position;
}

void
gil_statement_set_position (GilStatement * self,
                            gint position)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_NODE (self));

  self->position = position;
}
