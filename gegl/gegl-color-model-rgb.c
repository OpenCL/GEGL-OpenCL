#include "gegl-color-model-rgb.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-color.h"

static void class_init (GeglColorModelRgbClass * klass);
static void init (GeglColorModelRgb * self, GeglColorModelRgbClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static GeglColorAlphaSpace color_alpha_space (GeglColorModel * self_color_model);

static gpointer parent_class = NULL;

GType
gegl_color_model_rgb_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorModelRgbClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorModelRgb),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_MODEL , 
                                     "GeglColorModelRgb", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglColorModelRgbClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglColorModelClass *color_model_class = GEGL_COLOR_MODEL_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor;

  color_model_class->color_alpha_space = color_alpha_space;
  return;
}

static void 
init (GeglColorModelRgb * self, 
      GeglColorModelRgbClass * klass)
{
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL(self);

  self_color_model->colorspace = GEGL_COLOR_SPACE_RGB;
  self_color_model->is_additive = TRUE;
  self_color_model->color_space_name = NULL;
  self_color_model->num_colors = 3;
  
  self->red_index = 0;
  self->green_index = 1;
  self->blue_index = 2;
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
     self_color_model->alpha_channel = 3;
     self_color_model->num_channels = 4;
   }
  else
   {
     self_color_model->alpha_channel = -1;
     self_color_model->num_channels = 3;
   }

  return gobject;
}

static GeglColorAlphaSpace 
color_alpha_space (GeglColorModel * self_color_model)
{
  if (self_color_model->has_alpha)
    return GEGL_COLOR_ALPHA_SPACE_RGBA;
  else
    return GEGL_COLOR_ALPHA_SPACE_RGB;
}

/**
 * gegl_color_model_rgb_get_red_index:
 * @self: a #GeglColorModelRgb.
 *
 * Gets the index of the red channel.
 *
 * Returns: The index of of red channel.
 **/
gint 
gegl_color_model_rgb_get_red_index (GeglColorModelRgb * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL_RGB (self), (gint )0);
  return self->red_index; 
}

/**
 * gegl_color_model_rgb_get_green_index:
 * @self: a #GeglColorModelRgb.
 *
 * Gets the index of the green channel.
 *
 * Returns: The index of of green channel.
 **/
gint 
gegl_color_model_rgb_get_green_index (GeglColorModelRgb * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL_RGB (self), (gint )0);
    
  return self->green_index; 
}

/**
 * gegl_color_model_rgb_get_blue_index:
 * @self: a #GeglColorModelRgb.
 *
 * Gets the index of the blue channel.
 *
 * Returns: The index of of blue channel.
 **/
gint 
gegl_color_model_rgb_get_blue_index (GeglColorModelRgb * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL_RGB (self), (gint )0);
    
  return self->blue_index; 
}
