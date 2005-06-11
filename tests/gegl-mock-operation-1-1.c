#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-property.h"

#include "gegl-mock-operation-1-1.h"


enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_INPUT0,
  PROP_LAST
};

static void gegl_mock_operation_1_1_class_init (GeglMockOperation11Class *klass);
static void gegl_mock_operation_1_1_init       (GeglMockOperation11      *self);

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean evaluate     (GeglOperation   *operation,
                              const gchar  *output_prop);


G_DEFINE_TYPE (GeglMockOperation11, gegl_mock_operation_1_1, GEGL_TYPE_OPERATION)


static void
gegl_mock_operation_1_1_class_init (GeglMockOperation11Class *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->evaluate = evaluate;

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
                                                     100,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE |
                                                     GEGL_PROPERTY_INPUT));
}

static void
gegl_mock_operation_1_1_init (GeglMockOperation11 *self)
{
  GeglOperation   *operation       = GEGL_OPERATION(self);
  GObjectClass *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_property (operation,
                               g_object_class_find_property (object_class,
                                                             "output0"));
  gegl_operation_create_property (operation,
                               g_object_class_find_property (object_class,
                                                             "input0"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockOperation11 *self = GEGL_MOCK_OPERATION_1_1(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT0:
      g_value_set_int(value, self->output0);
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
  GeglMockOperation11 *self = GEGL_MOCK_OPERATION_1_1(gobject);
  switch (prop_id)
  {
    case PROP_INPUT0:
      self->input0 = g_value_get_int(value);
      break;
    default:
      break;
  }
}

static gboolean
evaluate (GeglOperation *operation,
         const gchar *output_prop)
{
  GeglMockOperation11 *self = GEGL_MOCK_OPERATION_1_1(operation);

  if(strcmp("output0", output_prop))
    return FALSE;

  self->output0 = 2 * self->input0;

  return  TRUE;
}
