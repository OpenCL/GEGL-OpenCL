#include "gegl-multi-image-op.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_SOURCE,
  PROP_M_SOURCE,
  PROP_LAST 
};

static void class_init (GeglMultiImageOpClass * klass);
static void init (GeglMultiImageOp * self, GeglMultiImageOpClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void validate_inputs  (GeglFilter *filter, GArray *collected_data);

static void compute_need_rects(GeglMultiImageOp *self);
static void compute_have_rect(GeglMultiImageOp *self);
static void compute_color_model (GeglMultiImageOp * self);

static gpointer parent_class = NULL;

GType
gegl_multi_image_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMultiImageOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMultiImageOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglMultiImageOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglMultiImageOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  filter_class->validate_inputs = validate_inputs;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_SOURCE,
               g_param_spec_object ("source",
                                    "Source",
                                    "The source image",
                                     GEGL_TYPE_OP,
                                     G_PARAM_CONSTRUCT_ONLY | 
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_SOURCE,
             gegl_param_spec_m_source ("m-source",
                                        "MSource",
                                        "The m-source image",
                                        G_PARAM_WRITABLE));

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
  GeglMultiImageOp *self = GEGL_MULTI_IMAGE_OP (gobject);
  switch (prop_id)
  {
    case PROP_SOURCE:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source");
        gegl_node_set_source(GEGL_NODE(self), source, index);  
      }
      break;
    case PROP_M_SOURCE:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source");
        gegl_node_set_m_source(GEGL_NODE(self), source, index, output);  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}


static void 
init (GeglMultiImageOp * self, 
      GeglMultiImageOpClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source");
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

/**
 * gegl_multi_image_op_compute_color_model:
 * @self: a #GeglMultiImageOp.
 *
 * Compute the color model of the multi_image op.
 *
 **/
void
gegl_multi_image_op_compute_color_model (GeglMultiImageOp * self)
{
  GeglMultiImageOpClass *klass;
  g_return_if_fail (GEGL_IS_MULTI_IMAGE_OP (self));
  klass = GEGL_MULTI_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_color_model)
    (*klass->compute_color_model)(self);
}

/**
 * gegl_multi_image_op_compute_have_rect:
 * @self: a #GeglMultiImageOp.
 *
 * Compute the have rect of this multi_image op.
 *
 **/
void      
gegl_multi_image_op_compute_have_rect (GeglMultiImageOp * self) 
{
  GeglMultiImageOpClass *klass;
  g_return_if_fail (GEGL_IS_MULTI_IMAGE_OP (self));
  klass = GEGL_MULTI_IMAGE_OP_GET_CLASS(self);

    if(klass->compute_have_rect)
      (*klass->compute_have_rect)(self);
}

void
gegl_multi_image_op_compute_need_rects(GeglMultiImageOp *self)
{
  GeglMultiImageOpClass *klass;
  g_return_if_fail (GEGL_IS_MULTI_IMAGE_OP (self));
  klass = GEGL_MULTI_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_need_rects)
    (*klass->compute_need_rects)(self);
}
