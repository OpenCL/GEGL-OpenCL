#include "gegl-image-op.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-color-model.h"

static void class_init (GeglImageOpClass * klass);
static void init (GeglImageOp * self, GeglImageOpClass * klass);
static void finalize(GObject * gobject);

static void validate_outputs (GeglFilter *filter);

static void compute_need_rects(GeglImageOp *self);
static void compute_have_rect(GeglImageOp *self);
static void compute_color_model (GeglImageOp * self);

static gpointer parent_class = NULL;

GType
gegl_image_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglImageOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglImageOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->compute_need_rects = compute_need_rects;
  klass->compute_have_rect = compute_have_rect;
  klass->compute_color_model = compute_color_model;

  filter_class->validate_outputs = validate_outputs;
}

static void 
init (GeglImageOp * self, 
      GeglImageOpClass * klass)
{
  gegl_op_add_output_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "dest");
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
validate_outputs (GeglFilter *filter)
{
  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(filter), "dest");
  GValue *value = gegl_data_get_value(output_data);
  GeglImage *image = (GeglImage*)g_value_get_object(value);
  if(!image)
    {
      GeglColorModel *color_model = 
        gegl_color_data_get_color_model(GEGL_COLOR_DATA(output_data));

      GeglImage *image = g_object_new(GEGL_TYPE_IMAGE, NULL);
      GeglRect rect;

      gegl_image_data_get_rect(GEGL_IMAGE_DATA(output_data), &rect);
      gegl_image_create_tile(image, color_model, &rect); 
      g_value_set_object(value, image);
      g_object_unref(image);
    }
}

/**
 * gegl_image_op_compute_color_model:
 * @self: a #GeglImageOp.
 *
 * Compute the color model of the image op.
 *
 **/
void
gegl_image_op_compute_color_model (GeglImageOp * self)
{
  GeglImageOpClass *klass;
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_color_model)
    (*klass->compute_color_model)(self);
}

static void
compute_color_model (GeglImageOp * self) 
{
  GeglColorModel *color_model;

  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
  GeglData *input_data = gegl_op_get_nth_input_data(GEGL_OP(self), 0);

  g_return_if_fail(GEGL_IS_IMAGE_DATA(output_data));
  g_return_if_fail(GEGL_IS_IMAGE_DATA(input_data));

  color_model = gegl_color_data_get_color_model(GEGL_COLOR_DATA(input_data));
  gegl_color_data_set_color_model(GEGL_COLOR_DATA(output_data), color_model);
}

/**
 * gegl_image_op_compute_have_rect:
 * @self: a #GeglImageOp.
 *
 * Compute the have rect of this image op.
 *
 **/
void      
gegl_image_op_compute_have_rect (GeglImageOp * self) 
{
  GeglImageOpClass *klass;
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

    if(klass->compute_have_rect)
      (*klass->compute_have_rect)(self);
}

static void
compute_have_rect(GeglImageOp *self) 
{
  gint i;
  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(self));
  GeglRect have_rect;

  gegl_rect_set(&have_rect, 0,0,0,0);

  gegl_log_debug(__FILE__, __LINE__, "compute_have_rect", 
                 "%s %p", G_OBJECT_TYPE_NAME(self), self); 

  for(i = 0; i < num_inputs; i++)
    {
      GeglRect bbox_rect;
      GeglData *input_data = gegl_op_get_nth_input_data(GEGL_OP(self), i); 
       
        if(input_data && 
           GEGL_IS_IMAGE_DATA(input_data))
        {
          GeglRect input_rect;
          gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &input_rect);
          gegl_rect_bounding_box (&bbox_rect, &input_rect, &have_rect); 
          gegl_rect_copy(&have_rect, &bbox_rect);
        }
    }

  gegl_image_data_set_rect(GEGL_IMAGE_DATA(output_data),&have_rect);

  gegl_log_debug(__FILE__, __LINE__,"compute_have_rect", 
                 "have rect is x y w h is %d %d %d %d", 
                 have_rect.x, have_rect.y, have_rect.w, have_rect.h);
}

void
gegl_image_op_compute_need_rects(GeglImageOp *self)
{
  GeglImageOpClass *klass;
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_need_rects)
    (*klass->compute_need_rects)(self);
}

void
compute_need_rects(GeglImageOp *self)
{
  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
  GeglImageData *output_image_data = GEGL_IMAGE_DATA(output_data);
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(self));
  gint i;

  for(i = 0 ; i < num_inputs; i++) 
    {
      GeglData *input_data = gegl_op_get_nth_input_data(GEGL_OP(self),i); 
      
      if(GEGL_IS_IMAGE_DATA(input_data))
        {
          GeglRect need_rect;         /* of the output image_data */
          GeglRect input_need_rect;   /* of the input image data */
          GeglImageData *input_image_data = GEGL_IMAGE_DATA(input_data);

          gegl_image_data_get_rect(output_image_data, &need_rect);
          gegl_image_data_get_rect(input_image_data, &input_need_rect);

          gegl_rect_bounding_box(&input_need_rect, 
                                 &need_rect, 
                                 &input_need_rect);

          gegl_log_debug(__FILE__, __LINE__, "compute_need_rect", 
                         "need rect for input %d is (x y w h) = (%d %d %d %d)", 
                         i, 
                         input_need_rect.x, 
                         input_need_rect.y, 
                         input_need_rect.w, 
                         input_need_rect.h); 

          gegl_image_data_set_rect(input_image_data, &input_need_rect);
        }
    }
}
