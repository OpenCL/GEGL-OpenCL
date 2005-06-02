#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-mock-image-filter.h"
#include "gegl-property.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT0,
  PROP_INPUT1,
  PROP_LAST
};

static void class_init (GeglMockImageFilterClass * klass);
static void init(GeglMockImageFilter *self, GeglMockImageFilterClass *klass);
static void finalize(GObject *gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gboolean evaluate (GeglFilter *filter, const gchar *output_prop);

static gpointer parent_class = NULL;

GType
gegl_mock_image_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockImageFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockImageFilter),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER,
                                     "GeglMockImageFilter",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GeglMockImageFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->evaluate = evaluate;

  g_object_class_install_property (gobject_class, PROP_OUTPUT,
               g_param_spec_object ("output",
                                 "Output",
                                 "An output property",
                                  GEGL_TYPE_MOCK_IMAGE,
                                  G_PARAM_READABLE |
                                  GEGL_PROPERTY_OUTPUT));

  g_object_class_install_property (gobject_class, PROP_INPUT0,
               g_param_spec_object ("input0",
                                 "Input0",
                                 "An input0 property",
                                  GEGL_TYPE_MOCK_IMAGE,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE |
                                  GEGL_PROPERTY_INPUT));

  g_object_class_install_property (gobject_class, PROP_INPUT1,
               g_param_spec_int ("input1",
                                 "Input1",
                                 "An input1 property",
                                  0,
                                  1000,
                                  500,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE |
                                  GEGL_PROPERTY_INPUT));
}

static void
init (GeglMockImageFilter * self,
      GeglMockImageFilterClass * klass)
{
  GeglFilter *filter = GEGL_FILTER(self);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "output"));
  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "input0"));
  gegl_filter_create_property(filter,
    g_object_class_find_property(gobject_class, "input1"));

  self->input0 = NULL;
  self->output = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglMockImageFilter * self = GEGL_MOCK_IMAGE_FILTER(gobject);

  if(self->output)
    g_object_unref(self->output);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockImageFilter *self = GEGL_MOCK_IMAGE_FILTER(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT:
      g_value_set_object(value, self->output);
      break;
    case PROP_INPUT0:
      g_value_set_object(value, self->input0);
      break;
    case PROP_INPUT1:
      g_value_set_int(value, self->input1);
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
  GeglMockImageFilter *self = GEGL_MOCK_IMAGE_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_INPUT0:
      self->input0 = g_value_get_object(value);
      break;
    case PROP_INPUT1:
      self->input1 = g_value_get_int(value);
      break;
    default:
      break;
  }
}

static gboolean
evaluate (GeglFilter *filter,
          const gchar *output_prop)
{
  GeglMockImageFilter *self = GEGL_MOCK_IMAGE_FILTER(filter);
  gint i;
  gint length;
  gint * output_data;
  gint * input0_data;

  if(strcmp("output", output_prop))
    return FALSE;

  /* Just throw the old one away */
  if(self->output)
    g_object_unref(self->output);

  g_object_get(self->input0, "length", &length, NULL);

  self->output = g_object_new(GEGL_TYPE_MOCK_IMAGE,
                              "length", length,
                              "default_pixel", 0,
                              NULL);

  output_data = gegl_mock_image_get_data(self->output);
  input0_data = gegl_mock_image_get_data(self->input0);

  for(i = 0; i < length; i++)
    output_data[i] = self->input1 * input0_data[i];

  /* Can release the ref to input */
  g_object_unref(self->input0);
  return  TRUE;
}
