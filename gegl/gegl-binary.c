#include "gegl-binary.h"
#include "gegl-attributes.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include "gegl-image-data.h"
#include "gegl-image-data-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_FADE,
  PROP_LAST 
};

static void class_init (GeglBinaryClass * klass);
static void init (GeglBinary * self, GeglBinaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);

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

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_FADE,
                                   g_param_spec_float ("fade",
                                                       "Fade",
                                                       "Fade on B.",
                                                       0.0, 
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

}

static void 
init (GeglBinary * self, 
      GeglBinaryClass * klass)
{
  g_object_set(self, "num_inputs", 2, NULL);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglBinary *binary = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_FADE:
      g_value_set_float(value, gegl_binary_get_fade(binary));  
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
  GeglBinary *binary = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_FADE:
      gegl_binary_set_fade(binary, g_value_get_float(value));  
      break;
    default:
      break;
  }
}

/**
 * gegl_binary_get_fade:
 * @self: a #GeglBinary.
 *
 * Gets the fade for the second input.
 *
 * Returns: the fade. 
 **/
gfloat
gegl_binary_get_fade (GeglBinary * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_BINARY (self), FALSE);

  return self->fade;
}


/**
 * gegl_binary_set_fade:
 * @self: a #GeglBinary.
 * @fade: the fade. 
 *
 * Sets the fade on the second input. 
 *
 **/
void 
gegl_binary_set_fade (GeglBinary * self, 
                      gfloat fade)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BINARY (self));

  self->fade = fade;
}


static void 
prepare (GeglFilter * filter, 
         GList * output_attributes,
         GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglBinary *self = GEGL_BINARY(filter);

  GeglAttributes *dest_attr = g_list_nth_data(output_attributes, 0);
  GeglImageData *dest = (GeglImageData*)g_value_get_object(dest_attr->value);
  GeglColorModel * dest_cm = gegl_image_data_get_color_model (dest);
  GeglColorSpace * dest_cs = gegl_color_model_color_space(dest_cm);
  GeglDataSpace * dest_ds = gegl_color_model_data_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglDataSpaceType type = gegl_data_space_data_space_type(dest_ds);
    GeglColorSpaceType space = gegl_color_space_color_space_type(dest_cs);
    GeglBinaryClass *klass = GEGL_BINARY_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
