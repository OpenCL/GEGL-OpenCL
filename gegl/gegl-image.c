#include "gegl-image.h"
#include "gegl-image-impl.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_COLOR_MODEL, 
  PROP_LAST 
};

static void class_init (GeglImageClass * klass);
static void init (GeglImage * self, GeglImageClass * klass);
static void finalize(GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void compute_derived_color_model (GeglImage * self, GList * input_color_models);

static gpointer parent_class = NULL;

GType
gegl_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglImage", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglImageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->compute_derived_color_model = compute_derived_color_model;
  klass->compute_have_rect = NULL;
  klass->compute_result_rect = NULL;
  klass->compute_preimage = NULL;

  g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                   g_param_spec_object ("colormodel",
                                                        "ColorModel",
                                                        "The GeglImage's colormodel",
                                                         GEGL_TYPE_COLOR_MODEL,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglImage * self, 
      GeglImageClass * klass)
{
  self->derived_color_model = NULL;
  self->color_model_is_derived = TRUE;
  self->dest = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglImage *self = GEGL_IMAGE(gobject);

  if(self->derived_color_model)
    g_object_unref(self->derived_color_model);

  if(self->dest) 
    g_object_unref(self->dest);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImage *self = GEGL_IMAGE(gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          GeglColorModel *cm = (GeglColorModel*)(g_value_get_object(value)); 
          gegl_image_set_color_model (self,cm);
        }
      else
        g_message("Cant set colormodel after construction");
      break;
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
  GeglImage *self = GEGL_IMAGE (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      g_value_set_object (value, (GObject*)gegl_image_color_model(self));
      break;
    default:
      break;
  }
}

/**
 * gegl_image_color_model:
 * @self: a #GeglImage.
 *
 * Gets the color model of this image.
 *
 * Returns: the #GeglColorModel, or NULL. 
 **/
GeglColorModel * 
gegl_image_color_model (GeglImage * self)
{
  GeglImageImpl * image_impl = GEGL_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  return gegl_image_impl_color_model(image_impl);
}

/**
 * gegl_image_set_color_model:
 * @self: a #GeglImage.
 * @cm: a #GeglColorModel.
 *
 * Sets the color model explicitly for this image. This takes precedence over
 * derived color models during graph evaluation. Set this when you want to
 * force an image to have a particular color model.
 *
 **/
void 
gegl_image_set_color_model (GeglImage * self, 
                            GeglColorModel * cm)
{
  GeglImageImpl * image_impl = GEGL_IMAGE_IMPL(GEGL_OP(self)->op_impl);

  if(cm)
    self->color_model_is_derived = FALSE;
  else
    self->color_model_is_derived = TRUE;

  gegl_image_impl_set_color_model(image_impl, cm);
}

GeglColorModel * 
gegl_image_derived_color_model (GeglImage * self)
{
  return self->derived_color_model;
}

void 
gegl_image_set_derived_color_model (GeglImage * self, 
                                       GeglColorModel * cm)
{
  if(self->derived_color_model)
    g_object_unref(self->derived_color_model);

  self->derived_color_model = cm;

  if(self->derived_color_model)
    g_object_ref(self->derived_color_model);
}

/**
 * gegl_image_compute_derived_color_model:
 * @self: a #GeglImage
 * @input_color_models: a #GeglColorModel list.
 *
 * Computes a derived #GeglColorModel for this image op.  Called by
 * #GeglImageMgr during graph evaluation.
 *
 **/
void
gegl_image_compute_derived_color_model (GeglImage * self, 
                                           GList * input_color_models)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_IMAGE (self));

   {
     GeglImageClass *klass = GEGL_IMAGE_GET_CLASS(self);

     if(klass->compute_derived_color_model)
        return (*klass->compute_derived_color_model)(self, input_color_models);
   }
}

static void 
compute_derived_color_model (GeglImage * self, 
                             GList * input_color_models)
{
  GeglImageImpl * image_impl = GEGL_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  GeglColorModel * cm = gegl_image_impl_color_model(image_impl);

  gegl_image_set_derived_color_model(self, cm);

  if(self->color_model_is_derived)
    gegl_image_impl_set_color_model(image_impl, cm);

}

/**
 * gegl_image_get_have_rect:
 * @self: a #GeglImage
 * @have_rect: returned rect.
 *
 * Gets the domain of definition.
 *
 **/
void 
gegl_image_get_have_rect (GeglImage * self, 
                             GeglRect * have_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));
   
  gegl_rect_copy (have_rect, &self->have_rect); 
}

/**
 * gegl_image_set_have_rect:
 * @self: a #GeglImage.
 * @have_rect: set to this.
 *
 * Sets the have rect of the image.
 *
 **/
void 
gegl_image_set_have_rect (GeglImage * self, 
                             GeglRect * have_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));
   
  gegl_rect_copy (&self->have_rect, have_rect);
}

/**
 * gegl_image_get_result_rect:
 * @self: a #GeglImage.
 * @result_rect: returned rect.   
 *
 * Gets the result region.
 *
 **/
void 
gegl_image_get_result_rect (GeglImage * self, 
                               GeglRect * result_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));
   
  gegl_rect_copy (result_rect, &self->result_rect);
}

/**
 * gegl_image_set_result_rect:
 * @self: a #GeglImage.
 * @result_rect: set to this.
 *
 * Sets the result rect of the image.
 *
 **/
void 
gegl_image_set_result_rect (GeglImage * self, 
                               GeglRect * result_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));
   
  gegl_rect_copy (&self->result_rect, result_rect);
}


/**
 * gegl_image_compute_have_rect:
 * @self: a #GeglImage.
 * @have_rect: the #GeglRect to compute.
 * @inputs: a list of the input #GeglRects.
 *
 * Computes a have_rect based on the inputs' have_rects.
 *
 **/
void 
gegl_image_compute_have_rect (GeglImage * self, 
                                 GeglRect * have_rect, 
                                 GList * inputs_have_rects)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  {
    GeglImageClass *klass = GEGL_IMAGE_GET_CLASS(self);

    if(klass->compute_have_rect)
      (*klass->compute_have_rect)(self,have_rect,inputs_have_rects);
  }
}

/**
 * gegl_image_compute_result_rect:
 * @self: a #GeglImage.
 * @result_rect: the #GeglRect to compute.
 * @need_rect: a #GeglRect.
 * @have_rect: a #GeglRect.
 *
 * Computes the result_rect based on a passed need rect and have rects.  Called
 * by #GeglImageMgr during graph evaluation. 
 *
 **/
void 
gegl_image_compute_result_rect (GeglImage * self, 
                                   GeglRect * result_rect, 
                                   GeglRect * need_rect, 
                                   GeglRect * have_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  {
    GeglImageClass *klass = GEGL_IMAGE_GET_CLASS(self);

    if(klass->compute_result_rect)
      (*klass->compute_result_rect)(self,result_rect,need_rect,have_rect);
  }
}

/**
 * gegl_image_get_need_rect:
 * @self: a #GeglImage.
 * @need_rect: returned rect.   
 *
 * Gets the region of interest.
 *
 **/
void 
gegl_image_get_need_rect (GeglImage * self, 
                             GeglRect * need_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE(self));
   
  gegl_rect_copy (need_rect, &self->need_rect);
}

/**
 * gegl_image_set_need_rect:
 * @self: a #GeglImage.
 * @need_rect: set to this.
 *
 * Sets the need rect of the image.
 *
 **/
void 
gegl_image_set_need_rect (GeglImage * self, 
                             GeglRect * need_rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));
   
  gegl_rect_copy (&self->need_rect, need_rect);
}

/**
 * gegl_image_compute_preimage:
 * @self: a #GeglImage.
 * @preimage: returned preimage rect on ith input. 
 * @rect: a rect on the #GeglOpImpl.
 * @i: which input.
 *
 * Computes the @preimage of @rect on the ith input. This is the inverse map
 * of @rect on the ith input. This routine is used to compute need rects.
 *
 **/
void 
gegl_image_compute_preimage (GeglImage * self, 
                                GeglRect * preimage, 
                                GeglRect * rect, 
                                gint i)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  {
    GeglImageClass *klass = GEGL_IMAGE_GET_CLASS(self);

    if(klass->compute_preimage)
      (*klass->compute_preimage)(self,preimage,rect,i);
  }
}

GeglImageImpl* 
gegl_image_get_dest (GeglImage *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE (self), NULL);

  if(self->dest)
    g_object_ref(self->dest);

  return self->dest;
} 

void             
gegl_image_set_dest (GeglImage *self,
                  GeglImageImpl *dest)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  if (self->dest)
    g_object_unref(self->dest);

  self->dest = dest;

  if (dest)
    g_object_ref(dest);
}
