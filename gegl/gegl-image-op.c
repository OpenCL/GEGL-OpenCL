#include "gegl-image-op.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-data.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-tile.h"

static void class_init (GeglImageOpClass * klass);
static void init (GeglImageOp * self, GeglImageOpClass * klass);
static void finalize(GObject * gobject);

static void validate_outputs (GeglFilter *filter, GList *data_outputs);

static void compute_have_rect(GeglImageOp *self, GeglRect *have_rect, GList * data_inputs);
static void compute_need_rect(GeglImageOp *self, GeglRect *input_need_rect, GeglRect * need_rect, gint i);
static GeglColorModel *compute_color_model (GeglImageOp * self, GList* data_inputs);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->compute_need_rect = compute_need_rect;
  klass->compute_have_rect = compute_have_rect;
  klass->compute_color_model = compute_color_model;

  filter_class->validate_outputs = validate_outputs;

  /* op properties */
  gegl_op_class_install_data_output_property(op_class, 
                                        gegl_param_spec_image("output-image", 
                                                              "OutputImage",
                                                              "Image output",
                                                              G_PARAM_PRIVATE));
}

static void 
init (GeglImageOp * self, 
      GeglImageOpClass * klass)
{
  gegl_op_append_output(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "output-image");

  self->image = g_object_new(GEGL_TYPE_IMAGE, NULL);
}

static void
finalize(GObject *gobject)
{
  GeglImageOp *self = GEGL_IMAGE_OP(gobject);

  if(self->image)
    g_object_unref(self->image);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_image_op_set_image:
 * @self: a #GeglImageOp.
 * @image: a #GeglImage.
 *
 * Sets the image of this image op.
 *
 **/
void
gegl_image_op_set_image (GeglImageOp * self,
                     GeglImage *image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));

  if(image)
   g_object_ref(image);

  if(self->image)
   g_object_unref(self->image);

  self->image = image;
}

/**
 * gegl_image_op_get_image:
 * @self: a #GeglImageOp.
 *
 * Gets the image of this image op.
 *
 * Returns: the #GeglImage of this image op. 
 **/
GeglImage *
gegl_image_op_get_image (GeglImageOp * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_OP (self), NULL);

  return self->image;
}

static void 
validate_outputs (GeglFilter *filter,
                  GList *data_outputs)
{
  GeglData *data_output = g_list_nth_data(data_outputs, 0);
  GValue *value = gegl_data_get_value(data_output);
  GeglImageOp *self = GEGL_IMAGE_OP(filter);
  GeglImage *image;

  if(G_VALUE_TYPE(value) != GEGL_TYPE_IMAGE)
    g_value_init(value, GEGL_TYPE_IMAGE);

  image = (GeglImage*)g_value_get_object(value);
  if(!image)
    {
      GeglColorModel *color_model = 
        gegl_color_data_get_color_model(GEGL_COLOR_DATA(data_output));
      GeglRect rect;

      gegl_image_data_get_rect(GEGL_IMAGE_DATA(data_output), &rect);
      gegl_image_create_tile(self->image, color_model, &rect); 

      g_value_set_object(value, self->image);
    }
}

/**
 * gegl_image_op_evaluate_color_model:
 * @self: a #GeglImageOp.
 * @data_inputs: list of #GeglData inputs.
 *
 * Evaluate the color model of this op.
 *
 **/
void
gegl_image_op_evaluate_color_model(GeglImageOp * self,
                                   GList * data_inputs)
{
  GeglData *data_output = gegl_op_get_data_output(GEGL_OP(self), 0); 
  GeglColorData *color_data = GEGL_COLOR_DATA(data_output);

  /* Compute the color model of this image */
  GeglColorModel *color_model = 
    gegl_image_op_compute_color_model(self, data_inputs); 

  /* Set the color data color model. */
  gegl_color_data_set_color_model(color_data, color_model);
}

/**
 * gegl_image_op_compute_color_model:
 * @self: a #GeglImageOp.
 * @data_inputs: list of #GeglData inputs.
 *
 * Compute the color model of the image op.
 *
 * Returns: a #GeglColorModel 
 **/
GeglColorModel *      
gegl_image_op_compute_color_model (GeglImageOp * self, 
                                   GList * data_inputs)
{
  GeglImageOpClass *klass;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_OP (self), NULL);
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_color_model)
    return (*klass->compute_color_model)(self, data_inputs);
  else
    return NULL;
}

static GeglColorModel * 
compute_color_model (GeglImageOp * self, 
                     GList * data_inputs)
{
  GeglColorData *color_data = (GeglColorData*)g_list_nth_data(data_inputs, 0);
  return gegl_color_data_get_color_model(color_data);
}

/**
 * gegl_image_op_evaluate_have_rect:
 * @self: a #GeglImageOp.
 * @data_inputs: list of #GeglData inputs.
 *
 * Evaluate the have rect for this op.
 *
 **/
void
gegl_image_op_evaluate_have_rect(GeglImageOp * self,
                                 GList * data_inputs)
{
  GeglData *data_output = gegl_op_get_data_output(GEGL_OP(self), 0); 
  GeglImageData *image_data = GEGL_IMAGE_DATA(data_output);
  GeglRect have_rect;
  GeglRect need_rect;
  GeglRect result_rect;

  /* Compute the have rect of this image */
  gegl_image_op_compute_have_rect(self, &have_rect, data_inputs); 

  /* Intersect with the need rect and store in result rect. */
  gegl_image_data_get_rect(image_data, &need_rect);
  gegl_rect_intersect(&result_rect, &need_rect, &have_rect);
  gegl_image_data_set_rect(image_data, &result_rect);
}

/**
 * gegl_image_op_compute_have_rect:
 * @self: a #GeglImageOp.
 * @have_rect: a #GeglRect to pass result back.
 * @data_inputs: list of #GeglData inputs.
 *
 * Compute the have rect of this image op.
 *
 **/
void      
gegl_image_op_compute_have_rect (GeglImageOp * self, 
                                 GeglRect * have_rect,
                                 GList * data_inputs)
{
  GeglImageOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_have_rect)
    (*klass->compute_have_rect)(self,have_rect,data_inputs);
}

static void
compute_have_rect(GeglImageOp *self, 
                  GeglRect * have_rect,
                  GList * data_inputs)
{
  gint i;
  gint num_inputs = g_list_length(data_inputs);
  GeglRect bbox_rect;

  gegl_rect_set(have_rect, 0,0,0,0);

  LOG_DEBUG("compute_have_rect", "%s %p", 
            G_OBJECT_TYPE_NAME(self), self); 


  for(i = 0; i < num_inputs; i++)
    {
      GeglData *data_input = (GeglData*) g_list_nth_data(data_inputs, i);

      if(data_input && GEGL_IS_IMAGE_DATA(data_input))
        {
          GeglRect input_rect;
          gegl_image_data_get_rect(GEGL_IMAGE_DATA(data_input), &input_rect);
          gegl_rect_bounding_box (&bbox_rect, &input_rect, have_rect); 
          gegl_rect_copy(have_rect, &bbox_rect);
        }
    }

  LOG_DEBUG("compute_have_rect", "have rect is x y w h is %d %d %d %d", 
             have_rect->x, have_rect->y, have_rect->w, have_rect->h);
}

/**
 * gegl_image_op_evaluate_need_rects:
 * @self: a #GeglImageOp.
 * @data_inputs: list of #GeglData inputs.
 *
 * Find need rects of inputs of this op.
 *
 **/
void
gegl_image_op_evaluate_need_rects(GeglImageOp *self, 
                                  GList *data_inputs)
{
  GeglData *data_output = gegl_op_get_data_output(GEGL_OP(self), 0);
  gint num_inputs = g_list_length(data_inputs);
  gint i;

  for(i = 0 ; i < num_inputs; i++) 
    {
      GeglData *data_input = (GeglData*)g_list_nth_data(data_inputs, i); 
      
      if(GEGL_IS_IMAGE_DATA(data_input))
        {
          GeglRect need_rect;
          GeglRect input_need_rect;

          GeglImageData *image_data = GEGL_IMAGE_DATA(data_output);
          GeglImageData *input_image_data = GEGL_IMAGE_DATA(data_input);

          gegl_image_data_get_rect(image_data, &need_rect);
          gegl_image_data_get_rect(input_image_data, &input_need_rect);

          /* Compute the need rect of this input */
          gegl_image_op_compute_need_rect(self, 
                                          &input_need_rect, 
                                          &need_rect, 
                                          i); 

          /* Store it in the inputs image data */
          gegl_image_data_set_rect(input_image_data, &input_need_rect);
        }
    }
}

/**
 * gegl_image_op_compute_need_rect:
 * @self: a #GeglImageOp.
 * @input_need_rect: The input need rect to compute.
 * @need_rect: #GeglRect need rect of this op.
 * @i: the input to compute need rect for.
 *
 * Compute the need rect of the ith input.
 *
 **/
void      
gegl_image_op_compute_need_rect(GeglImageOp * self,
                                GeglRect * input_need_rect,
                                GeglRect * need_rect,
                                gint i)
{
  GeglImageOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  klass = GEGL_IMAGE_OP_GET_CLASS(self);

  if(klass->compute_need_rect)
    (*klass->compute_need_rect)(self, input_need_rect, need_rect, i);
}

void      
compute_need_rect(GeglImageOp * self,
                  GeglRect * input_need_rect,
                  GeglRect * need_rect,
                  gint i)
{
  g_return_if_fail (self);
  g_return_if_fail (GEGL_IS_IMAGE_OP (self));
  g_return_if_fail (input_need_rect);
  g_return_if_fail (need_rect);

  gegl_rect_bounding_box(input_need_rect, need_rect, input_need_rect);
  LOG_DEBUG("compute_need_rect", "need rect for input %d is (x y w h) = (%d %d %d %d)", 
             i, 
             input_need_rect->x, 
             input_need_rect->y, 
             input_need_rect->w, 
             input_need_rect->h);
}
