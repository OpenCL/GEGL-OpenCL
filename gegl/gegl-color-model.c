#include "gegl-color-model.h"
#include "gegl-color.h"
#include "gegl-object.h"

enum
{
  PROP_0, 
  PROP_HAS_ALPHA,
  PROP_LAST 
};

static void class_init (GeglColorModelClass * klass);
static void init (GeglColorModel * self, GeglColorModelClass * klass);
static void finalize(GObject *gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;
static GHashTable * color_model_instances = NULL;

gboolean
gegl_color_model_register(gchar * color_model_name, 
                          GeglColorModel *color_model)
{
  g_return_val_if_fail (color_model != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (color_model), FALSE);

  {
    GeglColorModel * cm;

    if(!color_model_instances)
      {
        g_print("No ColorModel instances\n");
        return FALSE;
      }

    /* Get the singleton ref for this color model. */
    g_object_ref(color_model);

    gegl_color_model_set_color_space_name(color_model, color_model_name); 
    g_hash_table_insert(color_model_instances, 
                        (gpointer)gegl_color_model_get_color_space_name(color_model), 
                        (gpointer)color_model);

    cm = g_hash_table_lookup(color_model_instances, 
                             (gpointer)color_model_name);

    if(cm != color_model)
      return FALSE;
  }


  return TRUE;
} 

GeglColorModel *
gegl_color_model_instance(gchar *color_model_name)
{
  GeglColorModel * color_model = g_hash_table_lookup(color_model_instances, 
                                                     (gpointer)color_model_name);
  if(!color_model)
    {
      g_warning("%s not registered as ColorModel.\n", color_model_name);
      return NULL;
    }

  return color_model;
} 

GType
gegl_color_model_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorModelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorModel),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglColorModel", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglColorModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  klass->color_alpha_space = NULL;
  klass->color_model_type = NULL;
  klass->set_color = NULL;
  klass->convert_to_xyz = NULL;
  klass->convert_from_xyz = NULL;
  klass->get_convert_interface_name = NULL;

  g_object_class_install_property (gobject_class, PROP_HAS_ALPHA,
                                   g_param_spec_boolean ("hasalpha",
                                                      "Has Alpha",
                                                      "GeglColorModel has an alpha",
                                                      FALSE,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  color_model_instances = g_hash_table_new(g_str_hash,g_str_equal);
  return;
}

static void 
init (GeglColorModel * self, 
      GeglColorModelClass * klass)
{
  self->colorspace = GEGL_COLOR_SPACE_NONE;
  self->data_type = GEGL_NONE;
  self->bytes_per_channel = 0;
  self->bytes_per_pixel = 0; 
  self->num_channels = 0; 
  self->num_colors = 0;
  self->alpha_channel = 0;

  self->is_additive = FALSE;
  self->is_subtractive = FALSE;

  self->channel_names = NULL;
  self->color_space_name = NULL;
  self->alpha_string = NULL;
  self->channel_data_type_name = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglColorModel *self = GEGL_COLOR_MODEL(gobject);

  if(self->color_space_name)
    g_free(self->color_space_name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColorModel *self = GEGL_COLOR_MODEL (gobject);

  switch (prop_id)
  {
    case PROP_HAS_ALPHA:
        if(!GEGL_OBJECT(gobject)->constructed)
          gegl_color_model_set_has_alpha(self, g_value_get_boolean (value));
        else
          g_message("Cant set has alpha after construction\n");
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
  GeglColorModel *self = GEGL_COLOR_MODEL(gobject);

  switch (prop_id)
  {
    case PROP_HAS_ALPHA:
        g_value_set_boolean (value, gegl_color_model_has_alpha(self));
      break;
    default:
      break;
  }
}

/**
 * gegl_color_model_color_space:
 * @self: a #GeglColorModel
 *
 * Gets the color space of the color model. (eg RGB, GRAY)
 *
 * Returns: the #GeglColorSpace of this color model.
 **/
GeglColorSpace 
gegl_color_model_color_space (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (GeglColorSpace )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (GeglColorSpace )0);

  return self->colorspace; 
}

void
gegl_color_model_set_color_space_name (GeglColorModel * self, 
                                       gchar *name)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR_MODEL (self));
  g_return_if_fail (name != NULL);

  self->color_space_name = g_strdup(name); 
}

gchar *
gegl_color_model_get_color_space_name (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);
  return self->color_space_name; 
}

/**
 * gegl_color_model_data_type:
 * @self: a #GeglColorModel
 *
 * Gets the channel data type of the color model. (eg U8, U16, FLOAT)
 *
 * Returns: #GeglChannelDataType of the color model.  
 **/
GeglChannelDataType 
gegl_color_model_data_type (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (GeglChannelDataType )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (GeglChannelDataType )0);

  return self->data_type; 
}

/**
 * gegl_color_model_bytes_per_channel:
 * @self: a #GeglColorModel
 *
 * Gets the bytes per channel of the color model. 
 *
 * Returns: number of bytes per channel. 
 **/
gint 
gegl_color_model_bytes_per_channel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gint )0);
    
  return self->bytes_per_channel; 
}

/**
 * gegl_color_model_bytes_per_pixel:
 * @self: a #GeglColorModel
 *
 * Gets the bytes per pixel of the color model. 
 *
 * Returns: number of bytes per pixel. 
 **/
gint 
gegl_color_model_bytes_per_pixel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gint )0);

  return self->bytes_per_pixel; 
}

/**
 * gegl_color_model_num_channels:
 * @self: a #GeglColorModel
 *
 * Gets the number of channels of the color model. 
 *
 * Returns: number of channels. 
 **/
gint 
gegl_color_model_num_channels (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gint )0);

  return self->num_channels; 
}

/**
 * gegl_color_model_num_colors:
 * @self: a #GeglColorModel
 *
 * Gets the number of color channels of the color model. 
 *
 * Returns: number of color channels. 
 **/
gint 
gegl_color_model_num_colors (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gint )0);

  return self->num_colors; 
}

/**
 * gegl_color_model_has_alpha:
 * @self: a #GeglColorModel
 *
 * Gets whether this color model has an alpha channel. 
 *
 * Returns: TRUE if color model has an alpha channel. 
 **/
gboolean 
gegl_color_model_has_alpha (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gboolean )0);
    
  return self->has_alpha; 
}

void
gegl_color_model_set_has_alpha (GeglColorModel * self, 
                                gboolean has_alpha)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(GEGL_IS_COLOR_MODEL (self));
    
  self->has_alpha = has_alpha; 
}

/**
 * gegl_color_model_alpha_channel:
 * @self: a #GeglColorModel
 *
 * Gets the alpha channel index.
 *
 * Returns: Alpha channel index. 
 **/
gint 
gegl_color_model_alpha_channel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gint )0);
    
  return self->alpha_channel; 
}

/**
 * gegl_color_model_color_alpha_space:
 * @self: a #GeglColorModel
 *
 * Gets the #GeglColorAlphaSpace of the color model.(eg RGB, RGBA, GRAY,
 * GRAYA).
 *
 * Returns: The #GeglColorAlphaSpace of this color model.
 **/
GeglColorAlphaSpace 
gegl_color_model_color_alpha_space (GeglColorModel * self)
{
   GeglColorModelClass *klass;
   g_return_val_if_fail (self != NULL, (GeglColorAlphaSpace )0);
   g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (GeglColorAlphaSpace )0);
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->color_alpha_space)
      return (*klass->color_alpha_space)(self);
   else
      return (GeglColorAlphaSpace )(0);
}

/**
 * gegl_color_model_color_model_type:
 * @self: a #GeglColorModel
 *
 * Gets the #GeglColorModelType. (roughly #GeglColorAlphaSpace plus
 * #GeglChannelDataType)  
 *
 * Returns: The #GeglColorModelType of this color model.
 **/
GeglColorModelType 
gegl_color_model_color_model_type (GeglColorModel * self)
{
   GeglColorModelClass *klass;
   g_return_val_if_fail (self != NULL, (GeglColorModelType )0);
   g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (GeglColorModelType )0);
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->color_model_type)
      return (*klass->color_model_type)(self);
   else
      return (GeglColorModelType )(0);
}

/**
 * gegl_color_model_set_color:
 * @self: a #GeglColorModel
 * @color: a Color to set.
 * @constant: The #GeglColorConstant to use.
 *
 * Set @color to the @constant (eg white, black, green, red, etc) based on
 * the context of this color model.   
 *
 **/
void 
gegl_color_model_set_color (GeglColorModel * self, 
                            GeglColor * color, 
                            GeglColorConstant constant)
{
   GeglColorModelClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_COLOR_MODEL (self));
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->set_color)
      (*klass->set_color)(self,color,constant);
}

/**
 * gegl_color_model_convert_to_xyz:
 * @self: a #GeglColorModel
 * @dest_data: pointers to dest float xyz data.
 * @src_data: pointers to src data to convert from.
 * @width: number of pixels to process.
 *
 * Converts from this color model to XYZ. Every color model must implement
 * this.
 *
 **/
void 
gegl_color_model_convert_to_xyz (GeglColorModel * self, 
                                 gfloat ** dest_data, 
                                 guchar ** src_data, 
                                 gint width)
{
   GeglColorModelClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_COLOR_MODEL (self));
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->convert_to_xyz)
      (*klass->convert_to_xyz)(self,dest_data,src_data,width);
}

/**
 * gegl_color_model_convert_from_xyz:
 * @self: a #GeglColorModel
 * @dest_data: pointers to the dest to convert to.
 * @src_data: pointers to the src float xyz data.
 * @width: number of pixels to process.
 *
 * Converts from XYZ to this color model. Every color model must implement
 * this.
 *
 **/
void 
gegl_color_model_convert_from_xyz (GeglColorModel * self, 
                                   guchar ** dest_data, 
                                   gfloat ** src_data, 
                                   gint width)
{
   GeglColorModelClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_COLOR_MODEL (self));
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->convert_from_xyz)
      (*klass->convert_from_xyz)(self,dest_data,src_data,width);
}

/**
 * gegl_color_model_get_convert_interface_name:
 * @self: a #GeglColorModel
 *
 * Gets a the name of the converter interface that converts to data
 * of this color model.  
 *
 * Returns: a string that is the name of the converter interface. 
 **/
gchar * 
gegl_color_model_get_convert_interface_name (GeglColorModel * self)
{
   GeglColorModelClass *klass;
   g_return_val_if_fail (self != NULL, (gchar * )0);
   g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), (gchar * )0);
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->get_convert_interface_name)
      return (*klass->get_convert_interface_name)(self);
   else
      return (gchar * )(0);
}
