#include "gegl-fill.h"
#include "gegl-fill-impl.h"
#include "gegl-color.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_FILLCOLOR,
  PROP_LAST 
};

static void class_init (GeglFillClass * klass);
static void init (GeglFill * self, GeglFillClass * klass);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void compute_derived_color_model (GeglImage * self_image, GList * inputs);

static gpointer parent_class = NULL;

GType
gegl_fill_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFillClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFill),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglFill", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFillClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglImageClass *image_class = GEGL_IMAGE_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  image_class->compute_derived_color_model = compute_derived_color_model;

  g_object_class_install_property (gobject_class, PROP_FILLCOLOR,
                                   g_param_spec_object ("fillcolor",
                                                        "FillColor",
                                                        "The GeglFill's fill color",
                                                         GEGL_TYPE_COLOR,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));
  return;
}

static void 
init (GeglFill * self, 
      GeglFillClass * klass)
{
  GeglOp * self_op = GEGL_OP(self); 
  GeglNode * self_node = GEGL_NODE(self); 

  self_op->op_impl = g_object_new(GEGL_TYPE_FILL_IMPL, NULL);
  self_node->num_inputs = 0;

  return;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglFill *self = GEGL_FILL (gobject);

  switch (prop_id)
  {
    case PROP_FILLCOLOR:
        g_value_set_object (value, (GeglColor*)(gegl_fill_get_fill_color(self)));
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
  GeglFill *self = GEGL_FILL (gobject);

  switch (prop_id)
  {
    case PROP_FILLCOLOR:
        gegl_fill_set_fill_color (self,(GeglColor*)(g_value_get_object(value)));
      break;
    default:
      break;
  }
}

GeglColor * 
gegl_fill_get_fill_color (GeglFill * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILL (self), NULL);

  {
    GeglFillImpl * fill_impl = GEGL_FILL_IMPL(GEGL_OP(self)->op_impl);
    return gegl_fill_impl_get_fill_color(fill_impl);
  }
}

void
gegl_fill_set_fill_color (GeglFill * self, 
                          GeglColor *fill_color)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILL (self));
  {
    GeglFillImpl * fill_impl = GEGL_FILL_IMPL(GEGL_OP(self)->op_impl);
    gegl_fill_impl_set_fill_color(fill_impl, fill_color);
  }
}

static void
compute_derived_color_model (GeglImage * self_image, 
                             GList * inputs)
{
  GeglFillImpl * fill_impl = GEGL_FILL_IMPL(GEGL_OP(self_image)->op_impl);
  GeglColor *fill_color = gegl_fill_impl_get_fill_color(fill_impl);
  GeglColorModel *fill_cm = gegl_color_get_color_model(fill_color);

  gegl_image_set_derived_color_model(self_image, fill_cm);

  {
    GeglImageImpl * image_impl = GEGL_IMAGE_IMPL(GEGL_OP(self_image)->op_impl);
    gegl_image_impl_set_color_model(image_impl, fill_cm);
  }
}
