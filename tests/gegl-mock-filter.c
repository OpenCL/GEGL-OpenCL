#include "gegl-mock-filter.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-image-data.h"

enum
{
  PROP_0, 
  PROP_GLIB_FLOAT,
  PROP_GLIB_INT,
  PROP_CHANNEL_FLOAT,
  PROP_CHANNEL_UINT8,
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGBA_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_LAST 
};

static void class_init        (GeglMockFilterClass * klass);
static void init (GeglMockFilter * self, GeglMockFilterClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_mock_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglMockFilter", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_GLIB_FLOAT,
                                   g_param_spec_float ("glib-float",
                                                        "Glib-Float",
                                                        "A scalar float",
                                                         0.0,
                                                         1.0,
                                                         .5,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_GLIB_INT,
                                   g_param_spec_int ("glib-int",
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
                                                                  .5,
                                                                  G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL_UINT8,
                                   gegl_param_spec_channel_uint8 ("channel-uint8",
                                                                  "Channel-Uint8",
                                                                  "The channel uint8",
                                                                  0,
                                                                  255,
                                                                  128,
                                                                  G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
                                   gegl_param_spec_pixel_rgb_float ("pixel-rgb-float",
                                                                    "Pixel-Rgb-Float",
                                                                    "The pixel as rgb float",
                                                                    0.0, 1.0,
                                                                    0.0, 1.0,
                                                                    0.0, 1.0,
                                                                    .5, .5, .5,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGBA_FLOAT,
                                   gegl_param_spec_pixel_rgba_float ("pixel-rgba-float",
                                                                     "Pixel-Rgba-Float",
                                                                     "The pixel as rgba float",
                                                                      0.0, 1.0,
                                                                      0.0, 1.0,
                                                                      0.0, 1.0,
                                                                      0.0, 1.0,
                                                                      .5, .5, .5, .5,
                                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
                                   gegl_param_spec_pixel_rgb_uint8 ("pixel-rgb-uint8",
                                                                    "Pixel-Rgb-Uint8",
                                                                    "The pixel as rgb uint8",
                                                                    10, 245,
                                                                    10, 245,
                                                                    10, 245,
                                                                    128, 128, 128,
                                                                    G_PARAM_READWRITE));
}

static void 
init (GeglMockFilter * self, 
      GeglMockFilterClass * klass)
{
  gegl_op_add_output_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "output-image_op");

  self->channel = g_new0(GValue, 1); 
  g_value_init(self->channel, GEGL_TYPE_CHANNEL_FLOAT);

  self->pixel = g_new0(GValue, 1); 
  g_value_init(self->pixel, GEGL_TYPE_PIXEL_RGB_FLOAT);
}

static void
finalize(GObject *gobject)
{
  GeglMockFilter *self = GEGL_MOCK_FILTER(gobject);

  g_free(self->channel);
  g_free(self->pixel);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockFilter *self = GEGL_MOCK_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_GLIB_FLOAT:
      g_value_set_float(value, self->glib_float);
      break;
    case PROP_GLIB_INT:
      g_value_set_int(value, self->glib_int);
      break;
    case PROP_CHANNEL_FLOAT:
    case PROP_CHANNEL_UINT8:
      gegl_mock_filter_get_channel(self, value);
      break;
    case PROP_PIXEL_RGB_FLOAT:
      /*LOG_DIRECT("getting pixel-rgb-float");*/
      break;
    case PROP_PIXEL_RGBA_FLOAT:
      /*LOG_DIRECT("getting pixel-rgba-float");*/
      break;
    case PROP_PIXEL_RGB_UINT8:
      /*LOG_DIRECT("getting pixel-rgb-uint8");*/
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
  GeglMockFilter *self = GEGL_MOCK_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_GLIB_FLOAT:
      self->glib_float = g_value_get_float(value);
      break;
    case PROP_GLIB_INT:
      self->glib_int = g_value_get_int(value);
      break;
    case PROP_CHANNEL_FLOAT:
    case PROP_CHANNEL_UINT8:
      gegl_mock_filter_set_channel(self, (GValue*)value);
      break;
    case PROP_PIXEL_RGB_FLOAT:
      /*LOG_DIRECT("setting pixel-rgb-float");*/
      break;
    case PROP_PIXEL_RGBA_FLOAT:
      /*LOG_DIRECT("setting pixel-rgba-float");*/
      break;
    case PROP_PIXEL_RGB_UINT8:
      /*LOG_DIRECT("setting pixel-rgb-uint8");*/
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

void
gegl_mock_filter_get_channel (GeglMockFilter * self,
                              GValue *channel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_MOCK_FILTER (self));
  g_return_if_fail (g_value_type_transformable(G_VALUE_TYPE(self->channel), G_VALUE_TYPE(channel)));

  g_value_transform(self->channel, channel); 
}

void
gegl_mock_filter_set_channel (GeglMockFilter * self, 
                              GValue *channel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_MOCK_FILTER (self));

  g_value_unset(self->channel);
  g_value_init(self->channel, G_VALUE_TYPE(channel));
  g_value_copy(channel, self->channel); 
}

void
gegl_mock_filter_get_pixel (GeglMockFilter * self,
                            GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_MOCK_FILTER (self));
  g_return_if_fail (g_value_type_transformable(G_VALUE_TYPE(self->pixel), G_VALUE_TYPE(pixel)));

  g_value_transform(self->pixel, pixel); 
}

void
gegl_mock_filter_set_pixel (GeglMockFilter * self, 
                            GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_MOCK_FILTER (self));

  g_value_unset(self->pixel);
  g_value_init(self->pixel, G_VALUE_TYPE(pixel));
  g_value_copy(pixel, self->pixel); 
}
