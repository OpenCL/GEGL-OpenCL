#include "gegl-no-input.h"
#include "gegl-data.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-channel-space.h"
#include "gegl-image.h"
#include "gegl-image-op-interface.h"
#include "gegl-image-data.h"
#include "gegl-utils.h"

static void class_init (GeglNoInputClass * klass);
static void init (GeglNoInput * self, GeglNoInputClass * klass);

static void image_op_interface_init (gpointer ginterface, gpointer interface_data);
static void compute_have_rect (GeglImageOpInterface *interface);

static void prepare (GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_no_input_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglNoInputClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglNoInput),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      static const GInterfaceInfo image_op_interface_info = 
      { 
         (GInterfaceInitFunc) image_op_interface_init,
         NULL,  
         NULL
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglNoInput", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);

      g_type_add_interface_static (type, 
                                   GEGL_TYPE_IMAGE_OP_INTERFACE,
                                   &image_op_interface_info);

    }
    return type;
}

static void 
class_init (GeglNoInputClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglImageOpClass *image_op_class = GEGL_IMAGE_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;
}

static void
image_op_interface_init (gpointer ginterface,
                         gpointer interface_data)
{
  GeglImageOpInterfaceClass *interface = ginterface;

  g_assert (G_TYPE_FROM_INTERFACE (interface) == GEGL_TYPE_IMAGE_OP_INTERFACE);

  interface->compute_have_rect = compute_have_rect;
}

static void 
init (GeglNoInput * self, 
      GeglNoInputClass * klass)
{
}

static void
compute_have_rect (GeglImageOpInterface   *interface)
{
  g_print ("compute_have_rect: calling from no input\n");
  {
    GeglNoInput *self = GEGL_NO_INPUT(interface);
    GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
    GeglImageData *output_image_data = GEGL_IMAGE_DATA(output_data);

    GeglRect have_rect;
    gegl_rect_set(&have_rect, 0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT);
    gegl_image_data_set_rect(output_image_data,&have_rect);
  }
}

static void 
prepare (GeglFilter * filter)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglNoInput *self = GEGL_NO_INPUT(filter);
  GValue *dest_value = gegl_op_get_output_data_value(GEGL_OP(self), "dest"); 
  GeglImage *dest = (GeglImage*)g_value_get_object(dest_value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  GeglColorSpace * dest_color_space = gegl_color_model_color_space(dest_cm);
  GeglChannelSpace * dest_channel_space = gegl_color_model_channel_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglChannelSpaceType channel_type = 
      gegl_channel_space_channel_space_type(dest_channel_space);
    GeglColorSpaceType color_type = 
      gegl_color_space_color_space_type(dest_color_space);
    GeglNoInputClass *klass = GEGL_NO_INPUT_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, color_type, channel_type);

  }
}
