#include "gegl-unary.h"
#include "gegl-data.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-channel-space.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_SOURCE,
  PROP_LAST 
};

static void class_init (GeglUnaryClass * klass);
static void init (GeglUnary * self, GeglUnaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter);
static void validate_inputs  (GeglFilter *filter, GArray *collected_data);

static void compute_color_model (GeglImageOp * self);

static gpointer parent_class = NULL;

GType
gegl_unary_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglUnaryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglUnary),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglUnary", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglUnaryClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglImageOpClass *image_op_class = GEGL_IMAGE_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

  image_op_class->compute_color_model = compute_color_model;

  klass->get_scanline_func = NULL;

  g_object_class_install_property (gobject_class, PROP_SOURCE,
               g_param_spec_object ("source",
                                    "Source",
                                    "The source image",
                                     GEGL_TYPE_OP,
                                     G_PARAM_CONSTRUCT_ONLY | 
                                     G_PARAM_WRITABLE));
}

static void 
init (GeglUnary * self, 
      GeglUnaryClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
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
  GeglUnary *unary = GEGL_UNARY (gobject);
  switch (prop_id)
  {
    case PROP_SOURCE:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(unary), "source");
        gegl_node_set_source(GEGL_NODE(unary), source, index);  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                        GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);
  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "source");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "source", value);
  }
}

static void
compute_color_model (GeglImageOp * self) 
{
  GeglColorModel *color_model;

  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
  GeglData *input_data = gegl_op_get_input_data(GEGL_OP(self), "source");

  g_return_if_fail(GEGL_IS_IMAGE_DATA(output_data));
  g_return_if_fail(GEGL_IS_IMAGE_DATA(input_data));

  color_model = gegl_color_data_get_color_model(GEGL_COLOR_DATA(input_data));
  gegl_color_data_set_color_model(GEGL_COLOR_DATA(output_data), color_model);
}

static void 
prepare (GeglFilter * filter) 
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglUnary *self = GEGL_UNARY(filter);

  GValue *dest_value = gegl_op_get_output_data_value(GEGL_OP(self), "dest");
  GeglImage *dest = (GeglImage*)g_value_get_object(dest_value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  GeglColorSpace * dest_color_space = gegl_color_model_color_space(dest_cm);
  GeglChannelSpace * dest_channel_space = gegl_color_model_channel_space(dest_cm);

  GeglChannelSpaceType channel_space_type = 
    gegl_channel_space_channel_space_type(dest_channel_space);
  GeglColorSpaceType color_space_type = 
    gegl_color_space_color_space_type(dest_color_space);
  GeglUnaryClass *klass = GEGL_UNARY_GET_CLASS(self);

  /* Get the appropriate scanline func from subclass */
  if(klass->get_scanline_func)
    point_op->scanline_processor->func = 
      (*klass->get_scanline_func)(self, 
                                  color_space_type, 
                                  channel_space_type);

}
