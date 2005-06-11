#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-property.h"

#include "gegl-mock-operation-0-1.h"


enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_LAST
};

static void gegl_mock_operation_0_1_class_init (GeglMockOperation01Class *klass);
static void gegl_mock_operation_0_1_init       (GeglMockOperation01      *self);

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


G_DEFINE_TYPE (GeglMockOperation01, gegl_mock_operation_0_1, GEGL_TYPE_OPERATION)


static void
gegl_mock_operation_0_1_class_init (GeglMockOperation01Class *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->evaluate = evaluate;

  g_object_class_install_property (object_class, PROP_OUTPUT0,
                                   g_param_spec_int ("output0",
                                                     "Output0",
                                                     "An output0",
                                                     0,
                                                     1000,
                                                     0,
                                                     G_PARAM_READABLE |
                                                     GEGL_PROPERTY_OUTPUT));
}

static void
gegl_mock_operation_0_1_init (GeglMockOperation01 *self)
{
  GeglOperation *operation = GEGL_OPERATION(self);

  gegl_operation_create_property (operation,
                               g_object_class_find_property (G_OBJECT_GET_CLASS (self),
                                                             "output0"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockOperation01 *self = GEGL_MOCK_OPERATION_0_1(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT0:
      g_value_set_int(value, self->output0);
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
  /*GeglMockOperation01 *self = GEGL_MOCK_OPERATION_0_1(gobject);*/
  switch (prop_id)
  {
    default:
      break;
  }
}

static gboolean
evaluate (GeglOperation *operation,
         const gchar *output_prop)
{
  GeglMockOperation01 *self = GEGL_MOCK_OPERATION_0_1(operation);

  if(strcmp("output0", output_prop))
    return FALSE;

  self->output0 = 1;

  return  TRUE;
}
