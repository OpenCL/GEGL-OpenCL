#include "gegl-const-mult.h"
#include "gegl-const-mult-impl.h"
#include "gegl-image-mgr.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_MULTIPLIER,
  PROP_LAST 
};

static void class_init (GeglConstMultClass * klass);
static void init (GeglConstMult * self, GeglConstMultClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_const_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglConstMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglConstMult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglConstMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
init (GeglConstMult * self, 
      GeglConstMultClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  self_op->op_impl = g_object_new(GEGL_TYPE_CONST_MULT_IMPL, NULL);

  {
    GeglNode * self_node = GEGL_NODE(self); 
    GList * inputs = g_list_append(NULL, NULL);
    self_node->num_inputs = 1;
    gegl_node_set_inputs(self_node, inputs);
    g_list_free(inputs);
  }

  return;
}

static void 
class_init (GeglConstMultClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
   
  g_object_class_install_property (gobject_class,
                                   PROP_MULTIPLIER,
                                   g_param_spec_float ("multiplier",
                                                       "GeglConstMult Multiplier",
                                                       "Multiply input pixels by constant multiplier",
                                                       G_MINFLOAT, 
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  return;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglConstMult *self = GEGL_CONST_MULT (gobject);

  switch (prop_id)
  {
    case PROP_MULTIPLIER:
        g_value_set_float (value, gegl_const_mult_get_multiplier(self));
      break;
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
  GeglConstMult *self = GEGL_CONST_MULT (gobject);

  switch (prop_id)
  {
    case PROP_MULTIPLIER:
        gegl_const_mult_set_multiplier (self,g_value_get_float(value));
      break;
    default:
      break;
  }
}

gfloat
gegl_const_mult_get_multiplier (GeglConstMult * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_CONST_MULT (self), 0);
  {
    GeglConstMultImpl *const_mult_impl = GEGL_CONST_MULT_IMPL(GEGL_OP(self)->op_impl); 
    return gegl_const_mult_impl_get_multiplier(const_mult_impl);
  }
}

void
gegl_const_mult_set_multiplier (GeglConstMult * self, 
                                gfloat multiplier)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_CONST_MULT (self));
  {
    GeglConstMultImpl *const_mult_impl = GEGL_CONST_MULT_IMPL(GEGL_OP(self)->op_impl); 
    gegl_const_mult_impl_set_multiplier(const_mult_impl, multiplier);
  }
}
