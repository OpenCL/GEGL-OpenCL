#include "gegl-mock-properties-filter.h"
#include "gegl-channel-data.h"
#include "gegl-pixel-data.h"
#include "gegl-scalar-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_SCALAR_FLOAT,
  PROP_SCALAR_INT,
  PROP_CHANNEL_FLOAT,
  PROP_CHANNEL_UINT8,
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGBA_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_PIXEL_RGBA_UINT8,
  PROP_LAST 
};

static void class_init        (GeglMockPropertiesFilterClass * klass);
static void init (GeglMockPropertiesFilter * self, GeglMockPropertiesFilterClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_mock_properties_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockPropertiesFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockPropertiesFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglMockPropertiesFilter", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockPropertiesFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_SCALAR_FLOAT,
               g_param_spec_float ("scalar-float",
                                    "Glib-Float",
                                    "A scalar float",
                                     0.0,
                                     1.0,
                                     .5,
                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SCALAR_INT,
                 g_param_spec_int ("scalar-int",
                                   "Glib-Int",
                                   "A scalar int",
                                    0,
                                    2000,
                                    1000,
                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL_FLOAT,
     gegl_param_spec_channel_float ("channel-float",
                                    "Channel-Float",
                                    "The channel float",
                                    0.0,
                                    1.0,
                                    0.0,
                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL_UINT8,
     gegl_param_spec_channel_uint8 ("channel-uint8",
                                    "Channel-Uint8",
                                    "The channel uint8",
                                    0,
                                    255,
                                    0,
                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
     gegl_param_spec_pixel_rgb_float ("pixel-rgb-float",
                                      "Pixel-Rgb-Float",
                                      "The pixel as rgb float",
                                      0.0, 1.0,
                                      0.0, 1.0,
                                      0.0, 1.0,
                                      0.0, 0.0, 0.0,
                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGBA_FLOAT,
     gegl_param_spec_pixel_rgba_float ("pixel-rgba-float",
                                       "Pixel-Rgba-Float",
                                       "The pixel as rgba float",
                                        0.0, 1.0,
                                        0.0, 1.0,
                                        0.0, 1.0,
                                        0.0, 1.0,
                                        0.0, 0.0, 0.0, 0.0,
                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
     gegl_param_spec_pixel_rgb_uint8 ("pixel-rgb-uint8",
                                      "Pixel-Rgb-Uint8",
                                      "The pixel as rgb uint8",
                                      0, 255,
                                      0, 255,
                                      0, 255,
                                      0, 0, 0,
                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGBA_UINT8,
     gegl_param_spec_pixel_rgba_uint8 ("pixel-rgba-uint8",
                                      "Pixel-Rgba-Uint8",
                                      "The pixel as rgba uint8",
                                      0, 255,
                                      0, 255,
                                      0, 255,
                                      0, 255,
                                      0, 0, 0, 0,
                                      G_PARAM_READWRITE));

}

static void 
init (GeglMockPropertiesFilter * self, 
      GeglMockPropertiesFilterClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "scalar");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_CHANNEL_DATA, "channel");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_PIXEL_DATA, "pixel");
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockPropertiesFilter *self = GEGL_MOCK_PROPERTIES_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_SCALAR_FLOAT:
    case PROP_SCALAR_INT:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "scalar");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    case PROP_CHANNEL_FLOAT:
    case PROP_CHANNEL_UINT8:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "channel");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGBA_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
    case PROP_PIXEL_RGBA_UINT8:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
        g_param_value_convert(pspec, data_value, value, TRUE);
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
  GeglMockPropertiesFilter *self = GEGL_MOCK_PROPERTIES_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_SCALAR_FLOAT:
    case PROP_SCALAR_INT:
      gegl_op_set_input_data_value(GEGL_OP(self), "scalar", value);
      break;
    case PROP_CHANNEL_FLOAT:
    case PROP_CHANNEL_UINT8:
      gegl_op_set_input_data_value(GEGL_OP(self), "channel", value);
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGBA_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
    case PROP_PIXEL_RGBA_UINT8:
      gegl_op_set_input_data_value(GEGL_OP(self), "pixel", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}
