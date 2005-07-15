#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-pad.h"

#include "gegl-mock-operation-1-2.h"


enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_OUTPUT1,
  PROP_INPUT0,
  PROP_LAST
};

static void  gegl_mock_operation_1_2_class_init (GeglMockOperation12Class *klass);
static void  gegl_mock_operation_1_2_init       (GeglMockOperation12      *self);

static void  get_property (GObject      *gobject,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec);
static void  set_property (GObject      *gobject,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec);


G_DEFINE_TYPE (GeglMockOperation12, gegl_mock_operation_1_2, GEGL_TYPE_OPERATION)


static void
gegl_mock_operation_1_2_class_init (GeglMockOperation12Class *klass)
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
                                                     GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_OUTPUT1,
                                   g_param_spec_int ("output1",
                                                     "Output1",
                                                     "An output1 property",
                                                     0,
                                                     1000,
                                                     0,
                                                     G_PARAM_READABLE |
                                                     GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT0,
                                   g_param_spec_int ("input0",
                                                     "Input0",
                                                     "An input0 property",
                                                     0,
                                                     1000,
                                                     500,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE |
                                                     GEGL_PAD_INPUT));
}

static void
gegl_mock_operation_1_2_init (GeglMockOperation12 *self)
{
  GeglOperation   *operation       = GEGL_OPERATION (self);
  GObjectClass *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "output0"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "output1"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "input0"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockOperation12 *self = GEGL_MOCK_OPERATION_1_2(gobject);

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
  GeglMockOperation12 *self = GEGL_MOCK_OPERATION_1_2(gobject);
  switch (prop_id)
  {
    case PROP_INPUT0:
      self->input0 = g_value_get_int(value);
      break;
    default:
      break;
  }
}
