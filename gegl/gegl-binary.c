#include "gegl-binary.h"
#include "gegl-image-data.h"
#include "gegl-scalar-data.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-image.h"

enum
{
  PROP_0, 
  PROP_INPUT_IMAGE_A,
  PROP_INPUT_IMAGE_B,
  PROP_FADE,
  PROP_LAST 
};

static void class_init (GeglBinaryClass * klass);
static void init (GeglBinary * self, GeglBinaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void validate_inputs  (GeglFilter *filter, GList *collected_input_data_list);

static void prepare (GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_binary_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBinaryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBinary),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglBinary", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglBinaryClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_INPUT_IMAGE_A,
               g_param_spec_object ("input-image-a",
                                    "InputImageA",
                                    "The input image a",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_IMAGE_B,
               g_param_spec_object ("input-image-b",
                                    "InputImageB",
                                    "The input image b",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class, PROP_FADE, 
              g_param_spec_float ("fade",
                                  "Fade",
                                  "Fade on B.",
                                  0.0, 
                                  G_MAXFLOAT,
                                  1.0,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE));
}

static void 
init (GeglBinary * self, 
      GeglBinaryClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-a");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-b");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "fade");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglBinary *self = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_FADE:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "fade");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
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
  GeglBinary *self = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_INPUT_IMAGE_A:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "input-image-a");
        gegl_node_set_source(GEGL_NODE(self), input, index);  
      }
      break;
    case PROP_INPUT_IMAGE_B:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "input-image-b");
        gegl_node_set_source(GEGL_NODE(self), input, index);  
      }
      break;
    case PROP_FADE:
      gegl_op_set_input_data_value(GEGL_OP(self), "fade", value);
      break;
    default:
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                  GList *collected_input_data_list)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_input_data_list);

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "input-image-a");
    GeglData * data = g_list_nth_data(collected_input_data_list, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "input-image-a", value);
  }

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "input-image-b");
    GeglData * data = g_list_nth_data(collected_input_data_list, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "input-image-b", value);
  }
}

static void 
prepare (GeglFilter * filter) 
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglBinary *self = GEGL_BINARY(filter);
  GList * output_data_list = gegl_op_get_output_data_list(GEGL_OP(self));
  GeglData *output_data = g_list_nth_data(output_data_list, 0);
  GeglImage *dest = (GeglImage*)g_value_get_object(output_data->value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  GeglColorSpace * dest_color_space = gegl_color_model_color_space(dest_cm);
  GeglChannelSpace * dest_channel_space = gegl_color_model_channel_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglChannelSpaceType type = gegl_channel_space_channel_space_type(dest_channel_space);
    GeglColorSpaceType space = gegl_color_space_color_space_type(dest_color_space);
    GeglBinaryClass *klass = GEGL_BINARY_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
