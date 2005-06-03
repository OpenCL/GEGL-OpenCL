#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-property.h"

#include "gegl-mock-filter-2-1.h"


enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_INPUT0,
  PROP_INPUT1,
  PROP_LAST
};

static void  gegl_mock_filter_2_1_class_init (GeglMockFilter21Class *klass);
static void  gegl_mock_filter_2_1_init       (GeglMockFilter21      *self);

static void  get_property (GObject      *gobject,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec);
static void  set_property (GObject      *gobject,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec);


G_DEFINE_TYPE (GeglMockFilter21, gegl_mock_filter_2_1, GEGL_TYPE_FILTER)


static void
gegl_mock_filter_2_1_class_init (GeglMockFilter21Class * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  g_object_class_install_property (object_class, PROP_OUTPUT0,
                                   g_param_spec_int ("output0",
                                                     "Output0",
                                                     "An output0 property",
                                                     0,
                                                     1000,
                                                     0,
                                                     G_PARAM_READABLE |
                                                     GEGL_PROPERTY_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT0,
                                   g_param_spec_int ("input0",
                                                     "Input0",
                                                     "An input0 property",
                                                     0,
                                                     1000,
                                                     500,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE |
                                                     GEGL_PROPERTY_INPUT));

  g_object_class_install_property (object_class, PROP_INPUT1,
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
gegl_mock_filter_2_1_init (GeglMockFilter21 *self)
{
  GeglFilter   *filter       = GEGL_FILTER (self);
  GObjectClass *object_class = G_OBJECT_GET_CLASS (self);

  gegl_filter_create_property (filter,
                               g_object_class_find_property (object_class,
                                                             "output0"));
  gegl_filter_create_property (filter,
                               g_object_class_find_property (object_class,
                                                             "input0"));
  gegl_filter_create_property (filter,
                               g_object_class_find_property (object_class,
                                                             "input1"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockFilter21 *self = GEGL_MOCK_FILTER_2_1(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT0:
      g_value_set_int(value, self->output0);
      break;
    case PROP_INPUT0:
      g_value_set_int(value, self->input0);
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
  GeglMockFilter21 *self = GEGL_MOCK_FILTER_2_1(gobject);
  switch (prop_id)
  {
    case PROP_INPUT0:
      self->input0 = g_value_get_int(value);
      break;
    case PROP_INPUT1:
      self->input1 = g_value_get_int(value);
      break;
    default:
      break;
  }
}
