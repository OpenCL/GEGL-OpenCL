#include "gegl-comp.h"
#include "gegl-data.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-channel-space.h"
#include "gegl-image.h"
#include "gegl-image-iterator.h"
#include "gegl-image-data.h"
#include "gegl-param-specs.h"
#include "gegl-scalar-data.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_PREMULTIPLY,
  PROP_LAST 
};

static void class_init (GeglCompClass * klass);
static void init (GeglComp * self, GeglCompClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void validate_inputs  (GeglFilter *filter, GList *collected_input_data_list);

static void prepare (GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_comp_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCompClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglComp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglComp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglCompClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_PREMULTIPLY,
             g_param_spec_boolean ("premultiply",
                                   "Premultiply",
                                   "Premultiply the foreground.",
                                   FALSE,
                                   G_PARAM_CONSTRUCT | 
                                   G_PARAM_READWRITE));
}

static void 
init (GeglComp * self, 
      GeglCompClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-a");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-b");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "premultiply");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglComp *comp = GEGL_COMP (gobject);
  switch (prop_id)
  {
    case PROP_PREMULTIPLY:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(comp), "premultiply");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglComp *comp = GEGL_COMP (gobject);
  switch (prop_id)
  {
    case PROP_PREMULTIPLY:
      gegl_op_set_input_data_value(GEGL_OP(comp), "premultiply", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
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
  GeglComp *self = GEGL_COMP(filter);
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
    GeglCompClass *klass = GEGL_COMP_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
