#include "gegl-unary.h"
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
  PROP_LAST 
};

static void class_init (GeglUnaryClass * klass);
static void init (GeglUnary * self, GeglUnaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);

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
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
}

static void 
init (GeglUnary * self, 
      GeglUnaryClass * klass)
{
  g_object_set(self, "num_inputs", 1, NULL);
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
prepare (GeglFilter * filter, 
         GList * output_attributes,
         GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglUnary *self = GEGL_UNARY(filter);

  GeglAttributes *dest_attr = g_list_nth_data(output_attributes, 0);
  GeglImageData *dest = (GeglImageData*)g_value_get_object(dest_attr->value);
  GeglColorModel * dest_cm = gegl_image_data_get_color_model (dest);
  GeglColorSpace * dest_cs = gegl_color_model_color_space(dest_cm);
  GeglDataSpace * dest_ds = gegl_color_model_data_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglDataSpaceType type = gegl_data_space_data_space_type(dest_ds);
    GeglColorSpaceType space = gegl_color_space_color_space_type(dest_cs);
    GeglUnaryClass *klass = GEGL_UNARY_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
