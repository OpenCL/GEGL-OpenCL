#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_RGBA_FLOAT,
  PROP_RGB_FLOAT,
  PROP_RGBA_UINT8,
  PROP_RGB_UINT8,
  PROP_COMPONENTS,
  PROP_COLOR_COMPONENTS,
  PROP_COLOR_SPACE,
  PROP_LAST 
};

static void class_init (GeglColorClass * klass);
static void init (GeglColor *self, GeglColorClass * klass);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_color_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglColor),
        0,
        (GInstanceInitFunc) init,
        NULL,            
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglColor", 
                                     &typeInfo, 
                                     0);
    }


  return type;
}

static void 
class_init (GeglColorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_RGBA_FLOAT,
         gegl_param_spec_pixel_rgba_float ("rgba-float",
                                           "RgbaFloat",
                                           "A rgba float pixel",
                                           -G_MAXFLOAT, G_MAXFLOAT,
                                           -G_MAXFLOAT, G_MAXFLOAT,
                                           -G_MAXFLOAT, G_MAXFLOAT,
                                            0.0, 1.0,
                                            0.0, 0.0, 0.0, 1.0,
                                            G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RGB_FLOAT,
         gegl_param_spec_pixel_rgb_float ("rgb-float",
                                          "RgbFloat",
                                          "A rgb float pixel",
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                           0.0, 0.0, 0.0,
                                           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RGB_UINT8,
         gegl_param_spec_pixel_rgb_uint8 ("rgb-uint8",
                                          "RgbUint8",
                                          "A rgb uint8 pixel",
                                          0, 255,
                                          0, 255,
                                          0, 255,
                                          0, 0, 0,
                                          G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_RGB_UINT8,
         gegl_param_spec_pixel_rgba_uint8 ("rgba-uint8",
                                          "RgbaUint8",
                                          "A rgba uint8 pixel",
                                          0, 255,
                                          0, 255,
                                          0, 255,
                                          0, 255,
                                          0, 0, 0, 0,
                                          G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_COMPONENTS,
         gegl_param_spec_float_array ("components",
                                      "Components",
                                      "The components.",
                                       G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_COLOR_COMPONENTS,
         gegl_param_spec_float_array ("color-components",
                                      "ColorComponents",
                                      "The color components.",
                                       G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_COLOR_SPACE,
               g_param_spec_string ("color-space",
                                    "ColorSpace",
                                    "The color space.",
                                    "",
                                     G_PARAM_READABLE));
}

static void 
init (GeglColor * self, 
      GeglColorClass * klass)
{
  self->color_value = g_new0(GValue, 1); 
  g_value_init(self->color_value, GEGL_TYPE_PIXEL_RGB_FLOAT);
  g_value_set_pixel_rgb_float(self->color_value, 0.0, 0.0, 0.0);
}

static void
finalize(GObject *gobject)
{
  GeglColor *self = GEGL_COLOR(gobject);

  if(G_IS_VALUE(self->color_value))
    {
      g_value_unset(self->color_value);
      g_free(self->color_value);
    }

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColor * color = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_RGB_FLOAT:
    case PROP_RGBA_FLOAT:
      g_value_unset(color->color_value);
      g_value_init(color->color_value, value->g_type);
      g_value_copy(value, color->color_value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglColor * color = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_RGB_FLOAT:
    case PROP_RGBA_FLOAT:
    case PROP_RGB_UINT8:
    case PROP_RGBA_UINT8:
      g_param_value_convert(pspec, color->color_value, value, TRUE);
      break;
    case PROP_COMPONENTS:
      {
        GeglColorModel *cm = g_value_pixel_get_color_model(color->color_value);
        gint num_components = gegl_color_model_num_channels(cm);
        gfloat *data = (gfloat*)g_value_pixel_get_data(color->color_value); 
        g_value_set_float_array(value, num_components, data); 
      }
      break;
    case PROP_COLOR_COMPONENTS:
      {
        GeglColorModel *cm = g_value_pixel_get_color_model(color->color_value);
        gint num_colors = gegl_color_model_num_colors(cm);
        gfloat *data = (gfloat*)g_value_pixel_get_data(color->color_value); 
        g_value_set_float_array(value, num_colors, data); 
      }
      break;
    case PROP_COLOR_SPACE:
      {
        GeglColorModel *cm = g_value_pixel_get_color_model(color->color_value);
        GeglColorSpace *cs = gegl_color_model_color_space(cm);
        gchar *cs_name = gegl_color_space_name(cs); 
        g_value_set_string(value, cs_name); 
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}
