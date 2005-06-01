#include "gegl-mock-filter-1-2.h"
#include "../gegl/gegl-property.h"

enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_OUTPUT1,
  PROP_INPUT0,
  PROP_LAST
};

static void class_init (GeglMockFilter12Class * klass);
static void init(GeglMockFilter12 *self, GeglMockFilter12Class *klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_mock_filter_1_2_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockFilter12Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockFilter12),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER,
                                     "GeglMockFilter12",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GeglMockFilter12Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OUTPUT0,
               g_param_spec_int ("output0",
                                 "Output0",
                                 "An output0 property",
                                  0,
                                  1000,
                                  0,
                                  G_PARAM_READABLE |
                                  GEGL_PROPERTY_OUTPUT));

  g_object_class_install_property (gobject_class, PROP_OUTPUT1,
               g_param_spec_int ("output1",
                                 "Output1",
                                 "An output1 property",
                                  0,
                                  1000,
                                  0,
                                  G_PARAM_READABLE |
                                  GEGL_PROPERTY_OUTPUT));

  g_object_class_install_property (gobject_class, PROP_INPUT0,
               g_param_spec_int ("input0",
                                 "Input0",
                                 "An input0 property",
                                  0,
                                  1000,
                                  500,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE |
                                  GEGL_PROPERTY_INPUT));
}

static void
init (GeglMockFilter12 * self,
      GeglMockFilter12Class * klass)
{
  GeglFilter *filter = GEGL_FILTER(self);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "output0"));
  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "output1"));
  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "input0"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockFilter12 *self = GEGL_MOCK_FILTER_1_2(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT0:
      g_value_set_int(value, self->output0);
      break;
    case PROP_OUTPUT1:
      g_value_set_int(value, self->output1);
      break;
    case PROP_INPUT0:
      g_value_set_int(value, self->input0);
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
  GeglMockFilter12 *self = GEGL_MOCK_FILTER_1_2(gobject);
  switch (prop_id)
  {
    case PROP_INPUT0:
      self->input0 = g_value_get_int(value);
      break;
    default:
      break;
  }
}
