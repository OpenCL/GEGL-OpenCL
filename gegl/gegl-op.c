#include "gegl-op.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-visitor.h"
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
static void compute_need_rects(GeglOp *self, GList * input_value);

static void accept(GeglNode * node, GeglVisitor * visitor);

static void evaluate (GeglOp * self, GList * output_values, GList * input_values);
static void free_output_values(GeglOp * self);

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
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;

  klass->traverse = NULL;

  klass->compute_need_rects = compute_need_rects;
  klass->compute_have_rect = compute_have_rect;
  klass->evaluate = evaluate;

  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  klass->compute_derived_color_model = NULL;

  return;
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  GeglOp * self = GEGL_OP(gobject);

  free_output_values(self);
  g_list_free(self->output_values);

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
 * gegl_op_set_num_output_values:
 * @self: a #GeglOp.
 * @num_output_values: number of output values.
 *
 * Sets the number of output values for this op. 
 **/
void 
gegl_op_set_num_output_values(GeglOp * self, 
                              gint num_output_values)
{
  gint i;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  g_return_if_fail (num_output_values >= 0);

  for(i = 0; i < num_output_values; i++)
    {
      GValue * output_value = g_new0(GValue, 1);
      self->output_values = g_list_append(self->output_values, output_value);
    }
}

static void 
free_output_values(GeglOp * self)
{
  gint i;
  gint num_output_values = g_list_length(self->output_values);

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  for(i = 0; i < num_output_values; i++)
    {
      GValue * output_value = g_list_nth_data(self->output_values,i);
      g_value_unset(output_value);
      g_free(output_value);
    }
}

GList *
gegl_op_get_output_values(GeglOp * op)
{
  return op->output_values;
}

/**
 * gegl_op_compute_derived_color_model:
 * @self: a #GeglOp.
 * @input_values: the #GValues to use.
 *
 * Computes the derived color model of this op.
 **/
void      
gegl_op_compute_derived_color_model (GeglOp * self, 
                                     GList * input_values)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->compute_derived_color_model)
    (*klass->compute_derived_color_model)(self, input_values);
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
    GValue * output_value = gegl_op_get_nth_output_value(self, 0);
    g_value_get_image_data_rect(output_value, &rect); 

    LOG_DEBUG("gegl_op_evaluate", "evaluating %s %x", 
               G_OBJECT_TYPE_NAME(self), (guint)self); 
    LOG_DEBUG("gegl_op_evaluate", "input_values: %d output_values: %d",
               g_list_length(input_values), 
               g_list_length(output_values)); 
    LOG_DEBUG("gegl_op_evaluate", "result data rect: %d %d %d %d", 
               rect.x, rect.y, rect.w, rect.h); 
    if(GEGL_IS_IMAGE(self))
        {
          LOG_DEBUG("gegl_op_evaluate", "op->tile = %x output_value tile is %x", 
                     GEGL_IMAGE(self)->tile,
                     g_value_get_image_data_tile(output_value)); 
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
  GeglRect have_rect = {0,0,GEGL_DEFAULT_WIDTH,GEGL_DEFAULT_HEIGHT};
  GeglRect bbox_rect;
  GeglRect need_rect;
  GeglRect result_rect;
  gboolean first_time = TRUE;
  GValue * output_value;

  LOG_DEBUG("compute_have_rect", "%s %x", 
            G_OBJECT_TYPE_NAME(self), (guint)self); 

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
  output_value = gegl_op_get_nth_output_value(self, 0);
  g_value_get_image_data_rect(output_value, &need_rect);
  gegl_rect_intersect(&result_rect, &have_rect, &need_rect);

  /* Store the result rect in the output value. */
  g_value_set_image_data_rect(output_value, &result_rect);

  LOG_DEBUG("compute_have_rect", "have rect is x y w h is %d %d %d %d", 
             have_rect.x, have_rect.y, have_rect.w, have_rect.h);
  LOG_DEBUG("compute_have_rect", "result rect is x y w h is %d %d %d %d", 
             result_rect.x, result_rect.y, result_rect.w, result_rect.h);
}

void      
gegl_op_compute_need_rects(GeglOp * self,
                           GList * input_values)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->compute_need_rects)
    (*klass->compute_need_rects)(self, input_values);
}

void      
compute_need_rects (GeglOp * self, 
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
      if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
        {
          GeglRect image_data_rect;

          /* Get my output_value rect and set the rect of each input_value */ 
          GValue *output_value = gegl_op_get_nth_output_value(self, 0);
          g_value_get_image_data_rect(output_value, &image_data_rect);
          g_value_set_image_data_rect(input_value, &image_data_rect);

          LOG_DEBUG("compute_need_rects", "need rect for input %d is (x y w h) = (%d %d %d %d)", 
                     i, 
                     image_data_rect.x, 
                     image_data_rect.y, 
                     image_data_rect.w, 
                     image_data_rect.h);
        }
    }

  if(num_inputs ==0)
    LOG_DEBUG("compute_need_rects", "no inputs to compute need for"); 
}
    
GValue *
gegl_op_get_nth_output_value(GeglOp *self,
                             gint n) 
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);
  g_return_val_if_fail (n >=0 && n < g_list_length(self->output_values), NULL);

  return g_list_nth_data(self->output_values, n);
}

GValue *
gegl_op_get_nth_input_value(GeglOp * op,
                            gint n)
{
  GeglInputInfo * input_infos = gegl_node_get_input_infos(GEGL_NODE(op));
  GeglNode *input = input_infos[n].input; 
  gint input_index = input_infos[n].index;
  g_free(input_infos);

  if(input)
    return gegl_op_get_nth_output_value(GEGL_OP(input),input_index); 

  return NULL;
}

void      
gegl_op_apply(GeglOp * self)
{
  gegl_op_apply_image(self, NULL, NULL);
}

void      
gegl_op_apply_roi(GeglOp * self, 
                  GeglRect *roi)
{
  gegl_op_apply_image(self, NULL, roi);
}

void 
gegl_op_apply_image(GeglOp *self,
                    GeglOp * image,
                    GeglRect * roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  {
    GeglOp * filter = g_object_new(gegl_utils_get_filter_type(),
                                   "root", self,
                                   "image", image,
                                   "roi", roi,
                                   NULL);

    GeglOpClass *klass = GEGL_OP_GET_CLASS(filter);

    LOG_DEBUG("gegl_op_apply_image", "Creating filter for %s %x", 
               G_OBJECT_TYPE_NAME(self), (guint)self); 

    if(klass->traverse)
        (*klass->traverse)(filter);

    g_object_unref(filter);
  }
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_op(visitor, GEGL_OP(node));
}
