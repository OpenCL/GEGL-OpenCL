#include "gegl-mock-filter.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_FACTOR_FLOAT,
  PROP_FACTOR_UINT8,
  PROP_PIXEL_RGB_FLOAT,
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

  g_object_class_install_property (gobject_class, PROP_FACTOR_FLOAT,
                                   gegl_param_spec_float ("factor-float",
                                                          "Factor-Float",
                                                          "The factor as float",
                                                          0.0,
                                                          1.0,
                                                          .5,
                                                          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FACTOR_UINT8,
                                   gegl_param_spec_uint8 ("factor-uint8",
                                                          "Factor-Uint8",
                                                          "The factor as float",
                                                          0,
                                                          255,
                                                          128,
                                                          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
                                   gegl_param_spec_rgb_float ("pixel-rgb-float",
                                                              "Pixel-Rgb-Float",
                                                              "The pixel as float",
                                                              0.0, 1.0,
                                                              0.0, 1.0,
                                                              0.0, 1.0,
                                                              .5, .5, .5,
                                                              G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
                                   gegl_param_spec_rgb_uint8 ("pixel-rgb-uint8",
                                                              "Pixel-Rgb-Uint8",
                                                              "The pixel as uint8",
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
  g_object_set(self, "num_outputs", 0, NULL);
  g_object_set(self, "num_inputs", 0, NULL);

  self->value = g_new0(GValue, 1); 
  g_value_init(self->value, GEGL_TYPE_FLOAT);
}

static void
finalize(GObject *gobject)
{
  GeglMockFilter *self = GEGL_MOCK_FILTER(gobject);

  g_free(self->value);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    case PROP_FACTOR_FLOAT:
      /*LOG_DIRECT("getting factor-float");*/
      break;
    case PROP_FACTOR_UINT8:
      /*LOG_DIRECT("getting factor-uint8");*/
      break;
    case PROP_PIXEL_RGB_FLOAT:
      /*LOG_DIRECT("getting pixel-float");*/
      break;
    case PROP_PIXEL_RGB_UINT8:
      /*LOG_DIRECT("getting pixel-uint8");*/
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
  switch (prop_id)
  {
    case PROP_FACTOR_FLOAT:
      /*LOG_DIRECT("setting factor-float");*/
      break;
    case PROP_FACTOR_UINT8:
      /*LOG_DIRECT("setting factor-uint8");*/
      break;
    case PROP_PIXEL_RGB_FLOAT:
      /*LOG_DIRECT("setting pixel-float");*/
      break;
    case PROP_PIXEL_RGB_UINT8:
      /*LOG_DIRECT("setting pixel-uint8");*/
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}
