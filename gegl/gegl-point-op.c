#include "gegl-point-op.h"
#include "gegl-point-op-impl.h"
#include "gegl-utils.h"

static void class_init (GeglPointOpClass * klass);
static void compute_preimage (GeglImage * self_op, GeglRect * preimage, GeglRect * rect, gint i);
static void compute_have_rect (GeglImage * self_op, GeglRect *have_rect, GList * input_have_rects);
static void compute_result_rect (GeglImage * self_op, GeglRect * result_rect, GeglRect * need_rect, GeglRect *have_rect);
static void compute_derived_color_model (GeglImage * self, GList * input_color_models);

static gpointer parent_class = NULL;

GType
gegl_point_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPointOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPointOp),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE , 
                                     "GeglPointOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPointOpClass * klass)
{
  GeglImageClass *image_class = GEGL_IMAGE_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  image_class->compute_preimage = compute_preimage;
  image_class->compute_have_rect = compute_have_rect;
  image_class->compute_result_rect = compute_result_rect;
  image_class->compute_derived_color_model = compute_derived_color_model;

  return;
}

void             
gegl_point_op_get_nth_input_offset  (GeglPointOp * self, 
                                     gint i,
                                     GeglPoint *point)
{
  GeglPointOpImpl * point_op_impl = GEGL_POINT_OP_IMPL(GEGL_OP(self)->op_impl);
  gegl_point_op_impl_get_nth_input_offset (point_op_impl, i, point);
}

void             
gegl_point_op_set_nth_input_offset  (GeglPointOp * self, 
                                     gint i,
                                     GeglPoint * point)
{
  GeglPointOpImpl * point_op_impl = GEGL_POINT_OP_IMPL(GEGL_OP(self)->op_impl);
  gegl_point_op_impl_set_nth_input_offset (point_op_impl, i, point);
}

static void 
compute_have_rect (GeglImage * self_image, 
                   GeglRect * have_rect,
                   GList * inputs_have_rects)
{
  GeglPointOp *self = GEGL_POINT_OP(self_image);
  gint num_inputs = g_list_length(inputs_have_rects);
  GeglRect intersection;
  GeglRect input_have_rect;
  gint i;

  /* Start off as big as possible. */
  gegl_rect_set(have_rect, 0, 0, G_MAXINT, G_MAXINT);

  /* Take the intersection of all the inputs have_rects. */
  for (i = 0; i < num_inputs; i++)
    {
      GeglPoint input_offset;

      /* Get the have_rect of ith input, in its coords */
      gegl_rect_copy(&input_have_rect, (GeglRect*)g_list_nth_data(inputs_have_rects,i));
      gegl_point_op_get_nth_input_offset(self, i, &input_offset); 

      /* Put in the offsets for this op */
      input_have_rect.x += input_offset.x;
      input_have_rect.y += input_offset.y;

      /* Take intersection of input_have_rect and have_rect, store in have_rect */
      gegl_rect_intersect (&intersection, &input_have_rect, have_rect); 
      gegl_rect_copy(have_rect, &intersection);
    }
}

static void 
compute_preimage (GeglImage * self_image, 
                  GeglRect * preimage, 
                  GeglRect * rect, 
                  gint i)
{
  GeglPointOp *self = GEGL_POINT_OP(self_image); 
  GeglPoint input_offset;

  gegl_rect_copy (preimage, rect);
  gegl_point_op_get_nth_input_offset(self, i, &input_offset); 


  /* 
     Just take out the offset of this input,
     so put preimage in the inputs coord system.
  */ 
  preimage->x -= input_offset.x;
  preimage->y -= input_offset.y;
}

static void 
compute_result_rect (GeglImage * self_image, 
                     GeglRect * result_rect, 
                     GeglRect * need_rect, 
                     GeglRect * have_rect)
{
  /* By default just intersect result and need */
  gegl_rect_intersect (result_rect, need_rect, have_rect);
}

static void 
compute_derived_color_model (GeglImage * self, 
                             GList * input_color_models)
{
  GeglImageImpl * image_impl = GEGL_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  GeglColorModel * cm = (GeglColorModel*)(input_color_models->data);

  gegl_image_set_derived_color_model(self, cm);

  if(self->color_model_is_derived)
    gegl_image_impl_set_color_model(image_impl, cm);

}
