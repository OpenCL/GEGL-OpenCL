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
  PROP_LAST 
};

static void class_init (GeglUnaryClass * klass);
static void init (GeglUnary * self, GeglUnaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList *data_outputs, GList *data_inputs);
static void validate_inputs  (GeglFilter *filter, GList *data_inputs);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

  klass->get_scanline_func = NULL;
 
  /* op properties */
  gegl_op_class_install_data_input_property (op_class, 
                                        gegl_param_spec_image("input-image", 
                                                                   "InputImage",
                                                                   "The input image",
                                                                   G_PARAM_PRIVATE));
}

static void 
init (GeglUnary * self, 
      GeglUnaryClass * klass)
{
  /* Add the image input */
  gegl_op_append_input(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image");
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
  switch (prop_id)
  {
    default:
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                  GList *data_inputs)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, data_inputs);
}

static void 
prepare (GeglFilter * filter, 
         GList * data_outputs,
         GList * data_inputs)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglUnary *self = GEGL_UNARY(filter);

  GeglData *dest_data = g_list_nth_data(data_outputs, 0);
  GValue *dest_value = gegl_data_get_value(dest_data);
  GeglImage *dest = (GeglImage*)g_value_get_object(dest_value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  GeglColorSpace * dest_color_space = gegl_color_model_color_space(dest_cm);
  GeglChannelSpace * dest_channel_space = gegl_color_model_channel_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
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
}
