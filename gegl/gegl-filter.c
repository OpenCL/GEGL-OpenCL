#include "gegl-filter.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-visitor.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-image-data.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglFilterClass * klass);
static void init (GeglFilter * self, GeglFilterClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void init_attributes(GeglOp *op);
static void free_attributes(GeglFilter *self);

static void evaluate (GeglFilter * self, GeglAttributes * attributes, GList * input_attributes);

static void compute_have_rect(GeglFilter *self, GeglRect *have_rect, GList * input_have_rects);
static void compute_need_rect(GeglFilter *self, GeglRect *input_need_rect, GeglRect * need_rect, gint i);
static GeglColorModel *compute_derived_color_model (GeglFilter * filter, GList* input_color_models);

static void accept(GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

GType
gegl_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglFilter", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);
  GeglOpClass *op_class = GEGL_OP_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;

  op_class->init_attributes = init_attributes;

  klass->compute_need_rect = compute_need_rect;
  klass->compute_have_rect = compute_have_rect;
  klass->compute_derived_color_model = compute_derived_color_model;

  klass->evaluate = evaluate;

  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  klass->validate_inputs = NULL;
  klass->validate_outputs = NULL;

  return;
}

static void 
init (GeglFilter * self, 
      GeglFilterClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  GeglFilter *self = GEGL_FILTER(gobject);

  free_attributes(self);

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

static void
free_attributes(GeglFilter *self) 
{
  GeglOp *op = GEGL_OP(self);
  if(op->attributes)
    {
       g_value_unset(op->attributes->value);
       g_free(op->attributes->value);
       g_free(op->attributes);
       op->attributes = NULL;
    }
}

static void
init_attributes(GeglOp *op)
{
  GeglFilter *self = GEGL_FILTER(op);

  g_return_if_fail (op != NULL);
  g_return_if_fail (GEGL_IS_FILTER(op));

  /* Free any old attributes */
  free_attributes(self);

  /* Free and re-allocate the attributes */
  GEGL_OP_CLASS(parent_class)->init_attributes(op);

  /* Now allocate the attribute value and init */
  if(op->attributes)
    {
      op->attributes->value = g_new0(GValue, 1);
      g_value_init(op->attributes->value, GEGL_TYPE_IMAGE_DATA);
    }
}

/**
 * gegl_filter_compute_derived_color_model:
 * @self: a #GeglFilter.
 * @input_color_models: List of input color models.
 *
 * Computes the derived color model of the filter.
 *
 * Returns: the derived #GeglColorModel 
 **/
GeglColorModel *      
gegl_filter_compute_derived_color_model (GeglFilter * self, 
                                         GList * input_color_models)
{
  GeglFilterClass *klass;
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILTER (self), NULL);
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->compute_derived_color_model)
    return (*klass->compute_derived_color_model)(self, input_color_models);
  else
    return NULL;
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                             GList * input_color_models)
{
  GeglColorModel *cm = (GeglColorModel *)g_list_nth_data(input_color_models, 0);
  return cm; 
}

/**
 * gegl_filter_evaluate:
 * @self: a #GeglFilter.
 * @attributes: List of output attributes.
 * @input_attributes: List of input attributes.
 *
 * Evaluate the filter.
 * 
 **/
void      
gegl_filter_evaluate (GeglFilter * self, 
                      GeglAttributes * attributes,
                      GList * input_attributes)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->evaluate)
    (*klass->evaluate)(self, 
                       attributes, 
                       input_attributes);
}

void      
gegl_filter_validate_inputs (GeglFilter * self, 
                             GList * input_attributes)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->validate_inputs)
    (*klass->validate_inputs)(self, input_attributes);
}

void      
gegl_filter_validate_outputs (GeglFilter * self, 
                              GeglAttributes * attributes)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->validate_outputs)
    (*klass->validate_outputs)(self, attributes);
}

static void      
evaluate (GeglFilter * self, 
          GeglAttributes * attributes,
          GList * input_attributes)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->prepare)
    (*klass->prepare)(self, 
                      attributes, 
                      input_attributes);

  if(klass->process)
    (*klass->process)(self, 
                      attributes, 
                      input_attributes);

  if(klass->finish)
    (*klass->finish)(self, 
                     attributes, 
                     input_attributes);
}

/**
 * gegl_filter_compute_have_rect:
 * @self: a #GeglFilter.
 * @have_rect: a #GeglRect to pass result back.
 * @input_have_rects: List of input have rects.
 *
 * Computes the have rect of this filter.
 *
 **/
void      
gegl_filter_compute_have_rect (GeglFilter * self, 
                               GeglRect * have_rect,
                               GList * input_have_rects)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->compute_have_rect)
    (*klass->compute_have_rect)(self,have_rect,input_have_rects);
}

static void
compute_have_rect(GeglFilter *self, 
                  GeglRect * have_rect,
                  GList * input_have_rects)
{
  gint i;
  gint num_inputs = g_list_length(input_have_rects); 
  GeglRect bbox_rect;
  gegl_rect_set(have_rect, 0,0,0,0);

  LOG_DEBUG("compute_have_rect", "%s %p", 
            G_OBJECT_TYPE_NAME(self), self); 

  for(i = 0; i < num_inputs; i++)
    {
      GeglRect *input_rect =  (GeglRect*)g_list_nth_data(input_have_rects,i);
        
      if(input_rect)
        {
          gegl_rect_bounding_box (&bbox_rect, input_rect, have_rect); 
          gegl_rect_copy(have_rect, &bbox_rect);
        }
    }

  LOG_DEBUG("compute_have_rect", "have rect is x y w h is %d %d %d %d", 
             have_rect->x, have_rect->y, have_rect->w, have_rect->h);
}

/**
 * gegl_filter_compute_need_rect:
 * @self: a #GeglFilter.
 * @input_need_rect: an input need rect.
 * @need_rect: a #GeglRect to pass result back.
 * @i: the input to compute need rect for.
 *
 * Computes the need rect of the ith input.
 *
 **/
void      
gegl_filter_compute_need_rect(GeglFilter * self,
                              GeglRect * input_need_rect,
                              GeglRect * need_rect,
                              gint i)
{
  GeglFilterClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));
  klass = GEGL_FILTER_GET_CLASS(self);

  if(klass->compute_need_rect)
    (*klass->compute_need_rect)(self, input_need_rect, need_rect, i);
}

void      
compute_need_rect(GeglFilter * self,
                  GeglRect * input_need_rect,
                  GeglRect * need_rect,
                  gint i)
{
  g_return_if_fail (self);
  g_return_if_fail (GEGL_IS_FILTER (self));
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


static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_filter(visitor, GEGL_FILTER(node));
}
