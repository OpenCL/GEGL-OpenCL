#include "gil-constant.h"

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_INT,
  PROP_FLOAT,
  PROP_LAST 
};

static void class_init (GilConstantClass * klass);
static void init(GilConstant *self, GilConstantClass *klass);
static void finalize(GObject * self_object);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gil_constant_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilConstantClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilConstant),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_EXPRESSION, 
                                     "GilConstant", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GilConstantClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_TYPE,
                                   g_param_spec_int ("type",
                                                      "Type",
                                                      "Type of constant",
                                                      GIL_TYPE_NONE + 1,
                                                      GIL_TYPE_LAST - 1,
                                                      GIL_INT,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INT,
                                   g_param_spec_int ("int",
                                                      "Int Value",
                                                      "Integer value",
                                                      G_MININT,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FLOAT,
                                   g_param_spec_float("float",
                                                      "Float Value",
                                                      "Float constant",
                                                      -G_MAXFLOAT,
                                                      G_MAXFLOAT,
                                                      0.0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));
  return;
}

static void 
init (GilConstant * self, 
      GilConstantClass * klass)
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
  GilConstant *constant = GIL_CONSTANT (gobject);

  switch (prop_id)
  {
    case PROP_TYPE:
      {
        gil_constant_set_const_type(constant, (GilType)g_value_get_int(value));  
      }
      break;
    case PROP_INT:
      {
        gil_constant_set_int(constant, g_value_get_int(value));  
      }
      break;
    case PROP_FLOAT:
      {
        gil_constant_set_float(constant, g_value_get_float(value));  
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
  GilConstant *constant = GIL_CONSTANT (gobject);

  switch (prop_id)
  {
    case PROP_TYPE:
      {
        g_value_set_int(value, (gint)gil_constant_get_const_type(constant));  
      }
      break;
    case PROP_INT:
      {
        g_value_set_int(value, gil_constant_get_int(constant));  
      }
      break;
    case PROP_FLOAT:
      {
        g_value_set_float(value, gil_constant_get_float(constant));  
      }
      break;
    default:
      break;
  }
}

GilType
gil_constant_get_const_type (GilConstant * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_CONSTANT (self));

  return self->type;
}

void
gil_constant_set_const_type (GilConstant * self, 
                             GilType type)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_CONSTANT (self));
  g_return_if_fail (self->type == GIL_TYPE_NONE);

  self->type = type;
}

gint 
gil_constant_get_int (GilConstant * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GIL_IS_CONSTANT (self), 0);
  g_return_val_if_fail (self->type == GIL_INT, 0);

  return self->value.int_value;
}

void
gil_constant_set_int (GilConstant * self,
                      gint int_value)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_CONSTANT (self));

  if(self->type != GIL_INT)
    return;

  self->value.int_value = int_value;
}

gfloat 
gil_constant_get_float (GilConstant * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GIL_IS_CONSTANT (self), 0);
  g_return_val_if_fail (self->type == GIL_FLOAT, 0);

  return self->value.float_value;
}

void
gil_constant_set_float (GilConstant * self,
                        gfloat float_value)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GIL_IS_CONSTANT (self));

  if(self->type != GIL_FLOAT)
    return;

  self->value.float_value = float_value;
}
