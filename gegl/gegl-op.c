#include "gegl-op.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

#include "gegl-image.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglOpClass * klass);
static void init (GeglOp * self, GeglOpClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void compute_have_rect(GeglOp *self, GList * input_values);
static void compute_need_rect(GeglOp *self, GValue * input_value, gint i);

static void evaluate (GeglOp * self, GList * output_values, GList * input_values);
static void apply (GeglOp * self);

static gpointer parent_class = NULL;

GType
gegl_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NODE , 
                                     "GeglOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->apply = apply;

  klass->evaluate = evaluate;
  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  klass->compute_need_rect = compute_need_rect;
  klass->compute_have_rect = compute_have_rect;

  return;
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  self->output_value = g_new0(GValue, 1);
  g_value_init(self->output_value, GEGL_TYPE_IMAGE_DATA);
  return;
}

static void
finalize(GObject *gobject)
{
  GeglOp * self = GEGL_OP(gobject);

  g_value_unset(self->output_value);
  g_free(self->output_value);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
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

/**
 * gegl_op_evaluate:
 * @self: a #GeglOp.
 * @output_values: List of GValue output values.
 * @input_values: List of GValue input values.
 *
 *
 **/
void      
gegl_op_evaluate (GeglOp * self, 
                  GList * output_values,
                  GList * input_values)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  {
    GeglRect rect;
    g_value_get_image_data_rect(self->output_value, &rect); 

    LOG_DEBUG("gegl_op_evaluate", "evaluating %s %x", 
               G_OBJECT_TYPE_NAME(self), (guint)self); 
    LOG_DEBUG("gegl_op_evaluate", "input_values: %d output_values: %d",
               g_list_length(input_values), 
               g_list_length(output_values)); 
    LOG_DEBUG("gegl_op_evaluate", "result data rect: %d %d %d %d", 
               rect.x, rect.y, rect.w, rect.h); 
    if(GEGL_IS_IMAGE(self))
        {
          LOG_DEBUG("gegl_op_evaluate", "tile %x output value tile %x", 
                     GEGL_IMAGE(self)->tile,
                     g_value_get_image_data_tile(self->output_value)); 
        }

    if(klass->evaluate)
      (*klass->evaluate)(self, output_values, input_values);
                  
  }
}

static void      
evaluate (GeglOp * self, 
          GList * output_values,
          GList * input_values)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->prepare)
    (*klass->prepare)(self, output_values, input_values);

  if(klass->process)
    (*klass->process)(self, output_values, input_values);

  if(klass->finish)
    (*klass->finish)(self, output_values, input_values);
}

void      
gegl_op_compute_have_rect (GeglOp * self, 
                           GList * input_values)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->compute_have_rect)
    (*klass->compute_have_rect)(self,input_values);
}

static void
compute_have_rect(GeglOp *self, 
                  GList * input_values)
{
  gint i;
  gint num_inputs = g_list_length(input_values); 
  GeglRect have_rect = {0,0,-1,-1};
  GeglRect bbox_rect;
  GeglRect need_rect;
  GeglRect result_rect;
  gboolean first_time = TRUE;

  LOG_DEBUG("gegl_op_apply", "%s %x", 
            G_OBJECT_TYPE_NAME(self), (guint)self); 

  if(num_inputs > 0) 
    {
      for(i = 0; i < num_inputs; i++)
        {
          GValue *input_value =  (GValue*)g_list_nth_data(input_values,i);
            
          if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
            {
              GeglRect input_image_data_rect; 
              g_value_get_image_data_rect(input_value, 
                                          &input_image_data_rect);

              if(first_time)
                {
                  gegl_rect_copy(&have_rect, &input_image_data_rect);
                  first_time = FALSE;
                }
              else
                {
                  gegl_rect_bounding_box (&bbox_rect, 
                                          &input_image_data_rect, 
                                          &have_rect); 
                  gegl_rect_copy(&have_rect, &bbox_rect);
                }
            }
        }

      /* Find the result rect, intersection of have and need. */
      g_value_get_image_data_rect(self->output_value, &need_rect);
      gegl_rect_intersect(&result_rect, &have_rect, &need_rect);

      /* Store the result rect in the output value. */
      g_value_set_image_data_rect(self->output_value, &result_rect);

#if 0
      g_print("have rect is x y w h is %d %d %d %d\n", 
               result_rect.x, result_rect.y, result_rect.w, result_rect.h);
#endif
    }
}

void      
gegl_op_compute_need_rects (GeglOp * self, 
                            GList * input_values)
{
  gint i;
  gint num_inputs; 
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  num_inputs = g_list_length(input_values); 

  for(i = 0; i < num_inputs; i++)
    {
      GValue *input_value =  (GValue*)g_list_nth_data(input_values,i);
      gegl_op_compute_need_rect(self, input_value, i);
    }
}

void      
gegl_op_compute_need_rect(GeglOp * self,
                          GValue * input_value,
                          gint i)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->compute_need_rect)
    (*klass->compute_need_rect)(self,input_value, i);
}
    
static void
compute_need_rect(GeglOp *self, 
                  GValue * input_value,
                  gint i)
{
  if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
    {
      GeglRect image_data_rect;
      g_value_get_image_data_rect(self->output_value, &image_data_rect);
      g_value_set_image_data_rect(input_value, &image_data_rect);
    }
}

GValue *
gegl_op_get_output_value(GeglOp *self) 
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);

  return self->output_value;
}

void      
gegl_op_apply(GeglOp * self)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  LOG_DEBUG("gegl_op_apply", "%s %x", 
             G_OBJECT_TYPE_NAME(self), (guint)self); 

  if(klass->apply)
    (*klass->apply)(self);
}

static void 
apply (GeglOp * self)
{
  GeglOp * filter;

  LOG_DEBUG("apply", "Creating filter for %x", (guint)self); 

  filter = g_object_new(gegl_utils_get_filter_type(),
                        "root", self,
                        NULL);


  gegl_op_apply(filter);
  g_object_unref(filter);
}

void      
gegl_op_apply_image(GeglOp * self,
                    GeglOp * image,
                    GeglRect *roi)
{
  GeglOp * filter;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  LOG_DEBUG("op_apply_image", "Creating filter for %s %x", 
             G_OBJECT_TYPE_NAME(self), (guint)self); 

  filter = g_object_new(gegl_utils_get_filter_type(),
                        "root", self,
                        "image", image,
                        "roi", roi,
                        NULL);

  gegl_op_apply(filter);
  g_object_unref(filter);
}

void      
gegl_op_apply_roi(GeglOp * self, 
                  GeglRect *roi)
{
  GeglOp * filter;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  LOG_DEBUG("op_apply_roi", "Creating filter for %x", (guint)self); 

  filter = g_object_new(gegl_utils_get_filter_type(),
                        "root", self,
                        "roi", roi,
                        NULL);

  gegl_op_apply(filter);
  g_object_unref(filter);
}
