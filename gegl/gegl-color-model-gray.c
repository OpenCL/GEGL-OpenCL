#include "gegl-color-model-gray.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-color.h"

static void class_init (GeglColorModelGrayClass * klass);
static void init (GeglColorModelGray * self, GeglColorModelGrayClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static GeglColorAlphaSpace color_alpha_space (GeglColorModel * self_color_model);

static gpointer parent_class = NULL;

GType
gegl_color_model_gray_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorModelGrayClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorModelGray),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_MODEL, 
                                     "GeglColorModelGray", 
                                     &typeInfo,
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglColorModelGrayClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
   GeglColorModelClass *color_model_class = GEGL_COLOR_MODEL_CLASS(klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->constructor = constructor;

   color_model_class->color_alpha_space = color_alpha_space;
   return;
}

static void 
init (GeglColorModelGray * self, 
      GeglColorModelGrayClass * klass)
{
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL(self);

  self_color_model->colorspace = GEGL_COLOR_SPACE_GRAY;
  self_color_model->is_additive = TRUE;
  self_color_model->color_space_name = NULL;
  self_color_model->num_colors = 1;
  
  self->gray_index = 0;
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL(gobject);

  if (self_color_model->has_alpha)
   {
     self_color_model->alpha_channel = 1;
     self_color_model->num_channels = 2;
   }
  else
   {
     self_color_model->alpha_channel = -1;
     self_color_model->num_channels = 1;
   }

  return gobject;
}

static GeglColorAlphaSpace 
color_alpha_space (GeglColorModel * self_color_model)
{
  g_return_val_if_fail (self_color_model != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL_GRAY (self_color_model), (gint )0);

  if (self_color_model->has_alpha)
    return GEGL_COLOR_ALPHA_SPACE_GRAYA;
  else
    return GEGL_COLOR_ALPHA_SPACE_GRAY;
}

/**
 * gegl_color_model_gray_get_gray_index:
 * @self: a #GeglColorModelGray.
 *
 * Gets the index of the gray channel.
 *
 * Returns: The index of of gray channel.
 **/
gint 
gegl_color_model_gray_get_gray_index (GeglColorModelGray * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL_GRAY (self), (gint )0);
    
  return self->gray_index; 
}
