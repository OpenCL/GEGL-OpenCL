#include "gegl-blend.h"
#include "gegl-color-model.h"
#include "gegl-image.h"
#include "gegl-image-iterator.h"
#include "gegl-scalar-data.h"

enum
{
  PROP_0, 
  PROP_OPACITY,
  PROP_LAST 
};

static void class_init (GeglBlendClass * klass);
static void init (GeglBlend * self, GeglBlendClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_blend_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBlendClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBlend),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COMP , 
                                     "GeglBlend", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglBlendClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OPACITY,
                         g_param_spec_float ("opacity",
                                             "Opacity",
                                             "Opacity of this blend.",
                                             0.0, 
                                             1.0,
                                             1.0,
                                             G_PARAM_READWRITE | 
                                             G_PARAM_CONSTRUCT));
}

static void 
init (GeglBlend * self, 
      GeglBlendClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "opacity");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglBlend *blend = GEGL_BLEND (gobject);
  switch (prop_id)
  {
    case PROP_OPACITY:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(blend), "opacity");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglBlend *blend = GEGL_BLEND (gobject);
  switch (prop_id)
  {
    case PROP_OPACITY:
      gegl_op_set_input_data_value(GEGL_OP(blend), "opacity", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}
