#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-mock-image-operation.h"
#include "gegl-pad.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT0,
  PROP_INPUT1,
  PROP_LAST
};


static void  gegl_mock_image_operation_class_init (GeglMockImageOperationClass *klass);
static void  gegl_mock_image_operation_init       (GeglMockImageOperation      *self);

static void     finalize     (GObject      *gobject);
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
static void     associate    (GeglOperation *operation);



G_DEFINE_TYPE (GeglMockImageOperation, gegl_mock_image_operation, GEGL_TYPE_OPERATION)


static void
gegl_mock_image_operation_class_init (GeglMockImageOperationClass * klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->finalize     = finalize;

  operation_class->evaluate = evaluate;
  operation_class->associate = associate;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "An output property",
                                                        GEGL_TYPE_MOCK_IMAGE,
                                                        G_PARAM_READABLE |
                                                        GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT0,
                                   g_param_spec_object ("input0",
                                                        "Input0",
                                                        "An input0 property",
                                                        GEGL_TYPE_MOCK_IMAGE,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_INPUT1,
                                   g_param_spec_int ("input1",
                                                     "Input1",
                                                     "An input1 property",
                                                     0,
                                                     1000,
                                                     500,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE |
                                                     GEGL_PAD_INPUT));
}

static void
gegl_mock_image_operation_init (GeglMockImageOperation *self)
{
  self->input0 = NULL;
  self->output = NULL;
}

static void
associate (GeglOperation *self)
{
  GeglOperation   *operation = GEGL_OPERATION (self);
  GObjectClass *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "output"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "input0"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "input1"));
}

static void
finalize (GObject *object)
{
  GeglMockImageOperation *self = GEGL_MOCK_IMAGE_OPERATION (object);

  if (self->output)
    g_object_unref (self->output);

  G_OBJECT_CLASS (gegl_mock_image_operation_parent_class)->finalize (object);
}

static void
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockImageOperation *self = GEGL_MOCK_IMAGE_OPERATION (object);

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
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglMockImageOperation *self = GEGL_MOCK_IMAGE_OPERATION (object);

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
evaluate (GeglOperation *operation,
          const gchar *output_prop)
{
  GeglMockImageOperation *self = GEGL_MOCK_IMAGE_OPERATION(operation);
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
