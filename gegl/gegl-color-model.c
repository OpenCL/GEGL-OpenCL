#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-object.h"

enum
{
  PROP_0, 
  PROP_COLOR_SPACE,
  PROP_CHANNEL_TYPE_NAME,
  PROP_PIXEL_TYPE_NAME,
  PROP_HAS_ALPHA,
  PROP_HAS_Z,
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
gegl_color_model_register(GeglColorModel *color_model)
{
  g_return_val_if_fail (color_model != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (color_model), FALSE);

  {
    GeglColorModel * cm;
    gchar * name = gegl_color_model_name(color_model);

    if(!color_model_instances)
      {
        g_print("No ColorModel instances\n");
        return FALSE;
      }

    /* Get the singleton ref for this color model. */
    g_object_ref(color_model);

    g_hash_table_insert(color_model_instances, 
                        (gpointer)name, 
                        (gpointer)color_model);

    cm = g_hash_table_lookup(color_model_instances, (gpointer)name);

    if(cm != color_model)
      return FALSE;
  }

  return TRUE;
} 

GeglColorModel *
gegl_color_model_instance(gchar *name)
{
  GeglColorModel * color_model = g_hash_table_lookup(color_model_instances, 
                                                 (gpointer)name);
  if(!color_model)
    {
      g_warning("%s not registered as ColorModel.\n", name);
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
        NULL
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

  klass->create_storage = NULL;

  g_object_class_install_property (gobject_class, PROP_COLOR_SPACE,
                                   g_param_spec_object ("color_space",
                                                        "ColorSpace",
                                                        "The color space of the model",
                                                         GEGL_TYPE_COLOR_SPACE,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHANNEL_TYPE_NAME,
                                   g_param_spec_string("channel_type_name",
                                                       "ChannelTypeName",
                                                       "The channel typename of the model",
                                                       "",
                                                       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_TYPE_NAME,
                                   g_param_spec_string ("pixel_type_name",
                                                        "PixelTypeName",
                                                        "The pixel typename of the model",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HAS_ALPHA,
                                   g_param_spec_boolean ("has_alpha",
                                                      "Has Alpha",
                                                      "GeglColorModel has an alpha channel",
                                                      FALSE,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HAS_Z,
                                   g_param_spec_boolean ("has_z",
                                                      "Has Z",
                                                      "GeglColorModel has a z channel",
                                                      FALSE,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  color_model_instances = g_hash_table_new(g_str_hash,g_str_equal);
}

static void 
init (GeglColorModel * self, 
      GeglColorModelClass * klass)
{
  self->color_space = NULL;

  self->channel_type_name = NULL;
  self->pixel_type_name = NULL;

  self->bits_per_channel = NULL;
  self->bits_per_pixel = 0; 

  self->num_channels = 0; 
  self->num_colors = 0;

  self->has_alpha = FALSE;
  self->alpha_channel = -1;

  self->has_z = FALSE;
  self->z_channel = -1;

  self->channel_names = NULL;
  self->name = NULL;
}

static void
finalize(GObject *gobject)
{
  gint i;
  GeglColorModel *self = GEGL_COLOR_MODEL(gobject);

  g_free(self->bits_per_channel);
  g_free(self->name);

  g_free(self->channel_type_name);
  g_free(self->pixel_type_name);

  for(i = 0; i < self->num_channels; i++)
    g_free(self->channel_names[i]);

  g_free(self->channel_names);

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
    case PROP_COLOR_SPACE:
      self->color_space = GEGL_COLOR_SPACE(g_value_get_object(value));
      break;
    case PROP_CHANNEL_TYPE_NAME:
      self->channel_type_name = g_value_dup_string(value);
      break;
    case PROP_PIXEL_TYPE_NAME:
      self->pixel_type_name = g_value_dup_string(value);
      break;
    case PROP_HAS_ALPHA:
      gegl_color_model_set_has_alpha(self, g_value_get_boolean (value));
      break;
    case PROP_HAS_Z:
      gegl_color_model_set_has_z(self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
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
    case PROP_COLOR_SPACE:
      g_value_set_object (value, G_OBJECT(gegl_color_model_color_space(self)));
      break;
    case PROP_CHANNEL_TYPE_NAME:
      g_value_set_string (value, gegl_color_model_channel_type_name(self));
      break;
    case PROP_PIXEL_TYPE_NAME:
      g_value_set_string (value, gegl_color_model_pixel_type_name(self));
      break;
    case PROP_HAS_ALPHA:
      g_value_set_boolean (value, gegl_color_model_has_alpha(self));
      break;
    case PROP_HAS_Z:
      g_value_set_boolean (value, gegl_color_model_has_z(self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
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
GeglColorSpace*
gegl_color_model_color_space (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);

  return self->color_space; 
}

/**
 * gegl_color_model_channel_type_nmae:
 * @self: a #GeglColorModel
 *
 * Gets the channel type name of the color model. (eg uint8, float)
 *
 * Returns: the GType of the channel.
 **/
const gchar*
gegl_color_model_channel_type_name (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);

  return self->channel_type_name; 
}

/**
 * gegl_color_model_pixel_type:
 * @self: a #GeglColorModel
 *
 * Gets the pixel type of the color model. (eg rgb_uint8, rgb_float)
 *
 * Returns: the GType of the channel.
 **/
const gchar*
gegl_color_model_pixel_type_name (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);

  return self->pixel_type_name; 
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
 * gegl_color_model_bits_per_channel:
 * @self: a #GeglColorModel
 *
 * Gets the bits per channel of the color model. 
 *
 * Returns: array of bits per channel. 
 **/
gint* 
gegl_color_model_bits_per_channel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);
    
  return self->bits_per_channel; 
}

/**
 * gegl_color_model_bits_per_pixel:
 * @self: a #GeglColorModel
 *
 * Gets the bits per pixel of the color model. 
 *
 * Returns: number of bits per pixel. 
 **/
gint 
gegl_color_model_bits_per_pixel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), 0);

  return self->bits_per_pixel; 
}

/**
 * gegl_color_model_name:
 * @self: a #GeglColorModel
 *
 * Gets the color model name. 
 *
 * Returns: name of the color model. 
 **/
gchar *
gegl_color_model_name (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);

  return self->name; 
}

/**
 * gegl_color_model_color_model_name:
 * @self: a #GeglColorModel
 *
 * Gets the color models channel names . 
 *
 * Returns: array of names of the channels of the color model. 
 **/
gchar**
gegl_color_model_channel_names (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);

  return self->channel_names; 
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
 * gegl_color_model_has_z:
 * @self: a #GeglColorModel
 *
 * Gets whether this color model has an z channel. 
 *
 * Returns: TRUE if color model has an z channel. 
 **/
gboolean 
gegl_color_model_has_z (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), 0);
    
  return self->has_z; 
}

void
gegl_color_model_set_has_z (GeglColorModel * self, 
                          gboolean has_z)
{
  g_return_if_fail(self != NULL);
  g_return_if_fail(GEGL_IS_COLOR_MODEL (self));
    
  self->has_z = has_z; 
}

/**
 * gegl_color_model_z_channel:
 * @self: a #GeglColorModel
 *
 * Gets the z channel index.
 *
 * Returns: Alpha channel index. 
 **/
gint 
gegl_color_model_z_channel (GeglColorModel * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), 0);
    
  return self->z_channel; 
}

/**
 * gegl_color_model_create_storage:
 * @self: a #GeglColorModel
 * @width: width.
 * @height: height.
 *
 * Create storage compatible with this color model.
 *
 * Returns: a GeglStorage.
 **/
GeglStorage *
gegl_color_model_create_storage (GeglColorModel * self, 
                               gint width, 
                               gint height)
{
   GeglColorModelClass *klass;
   g_return_val_if_fail (self != NULL, NULL);
   g_return_val_if_fail (GEGL_IS_COLOR_MODEL (self), NULL);
   klass = GEGL_COLOR_MODEL_GET_CLASS(self);

   if(klass->create_storage)
      return (*klass->create_storage)(self, width, height);

   return NULL;
}
