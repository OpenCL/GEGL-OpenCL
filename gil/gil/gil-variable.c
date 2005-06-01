#include "gil-variable.h"

enum
{
  PROP_0,
  PROP_VARIABLE,
  PROP_TYPE,
  PROP_LAST
};

static void class_init (GilVariableClass * klass);
static void init(GilVariable *self, GilVariableClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_variable_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilVariableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilVariable),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_EXPRESSION,
                                     "GilVariable",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GilVariableClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_VARIABLE,
                                   g_param_spec_string ("variable",
                                                        "Variable",
                                                        "This is the Variable name",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TYPE,
                                   g_param_spec_int ("type",
                                                      "Type",
                                                      "This is the Variable type",
                                                      GIL_TYPE_NONE,
                                                      GIL_TYPE_LAST - 1,
                                                      GIL_TYPE_NONE,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  return;
}

static void
init (GilVariable * self,
      GilVariableClass * klass)
{
  self->type = GIL_TYPE_NONE;
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
  GilVariable * variable = GIL_VARIABLE(gobject);
  switch (prop_id)
  {
    case PROP_VARIABLE:
      gil_variable_set_name(variable, g_value_get_string(value));
      break;
    case PROP_TYPE:
      gil_variable_set_variable_type(variable, g_value_get_int(value));
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
  GilVariable * variable = GIL_VARIABLE(gobject);
  switch (prop_id)
  {
    case PROP_VARIABLE:
      g_value_set_string(value, gil_variable_get_name(variable));
      break;
    case PROP_TYPE:
      g_value_set_int(value, gil_variable_get_variable_type(variable));
      break;
    default:
      break;
  }
}

GilType
gil_variable_get_variable_type (GilVariable * self)
{
  g_return_val_if_fail (self, GIL_TYPE_NONE);
  g_return_val_if_fail (GIL_IS_VARIABLE (self), GIL_TYPE_NONE);

  return self->type;
}

void
gil_variable_set_variable_type (GilVariable * self,
                                GilType type)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_VARIABLE (self));
  g_return_if_fail (type >= GIL_TYPE_NONE && type < GIL_TYPE_LAST);

  self->type = type;
}

void
gil_variable_set_name (GilVariable * self,
                       const gchar * name)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_VARIABLE (self));

  self->name = g_strdup(name);
}

G_CONST_RETURN gchar*
gil_variable_get_name (GilVariable * self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GIL_IS_VARIABLE (self), NULL);

  return self->name;
}
