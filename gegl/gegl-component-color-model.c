#include "gegl-component-color-model.h"
#include "gegl-color-space.h"
#include "gegl-component-storage.h"
#include "gegl-object.h"
#include "gegl-channel-value-types.h"
#include "gegl-pixel-value-types.h"

static void class_init (GeglComponentColorModelClass * klass);
static void init (GeglComponentColorModel * self, GeglComponentColorModelClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static GeglStorage * create_storage (GeglColorModel * self, gint width, gint height);

static gpointer parent_class = NULL;

GType
gegl_component_color_model_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglComponentColorModelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglComponentColorModel),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_MODEL, 
                                     "GeglComponentColorModel", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglComponentColorModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglColorModelClass * color_model_class = GEGL_COLOR_MODEL_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor;

  color_model_class->create_storage = create_storage;
}

static void 
init (GeglComponentColorModel * self, 
      GeglComponentColorModelClass * klass)
{
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  gint i;
  gint *bits;
  gint num_colors;
  gint num_channels;
  GType channel_type;

  PixelValueInfo *pixel_value_info;
  ChannelValueInfo *channel_value_info;

  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglColorModel *color_model = GEGL_COLOR_MODEL(gobject);

  GType pixel_type = g_type_from_name(color_model->pixel_type_name);
  pixel_value_info = g_type_get_qdata(pixel_type, g_quark_from_string("pixel_value_info"));

  color_model->channel_type_name = g_strdup(pixel_value_info->channel_type_name);
  channel_type = g_type_from_name(color_model->channel_type_name);

  /* Remove this! */
  color_model->color_space = gegl_color_space_instance(pixel_value_info->color_space_name); 

  channel_value_info =  g_type_get_qdata(channel_type, g_quark_from_string("channel_value_info"));
#if 0
  color_model->channel_space = gegl_channel_space_instance(channel_value_info->channel_space_name); 
#endif

  num_colors = gegl_color_space_num_components(color_model->color_space);
  num_channels = num_colors;

  color_model->num_colors = num_colors;
  color_model->has_alpha = pixel_value_info->has_alpha;

  if(color_model->has_alpha)
    {
      color_model->alpha_channel = num_channels; 
      num_channels++;
    }

  color_model->num_channels = num_channels;

  color_model->bits_per_channel = g_new(gint, color_model->num_channels);

  for(i = 0; i < num_colors; i++)
    color_model->bits_per_channel[i] = channel_value_info->bits_per_channel;

  if(color_model->has_alpha)
    color_model->bits_per_channel[color_model->alpha_channel] = channel_value_info->bits_per_channel; 

  color_model->bits_per_pixel = 0;
  for(i = 0; i < color_model->num_channels; i++)
    color_model->bits_per_pixel += color_model->bits_per_channel[i];

  color_model->name = g_strdup(pixel_value_info->color_model_name);
  color_model->channel_names = g_new(gchar*, color_model->num_channels);

  for(i = 0; i < color_model->num_colors; i++) 
    {
      gchar * channel_name = gegl_color_space_component_name(color_model->color_space, i);
      color_model->channel_names[i] = g_strdup(channel_name);
    }
    
  if(color_model->has_alpha)
    color_model->channel_names[color_model->alpha_channel] = g_strdup("alpha"); 

  return gobject;
}

static GeglStorage * 
create_storage (GeglColorModel * color_model, 
                gint width, 
                gint height)
{
  GType channel_type = g_type_from_name(color_model->channel_type_name);
  ChannelValueInfo *channel_value_info =  g_type_get_qdata(channel_type, g_quark_from_string("channel_value_info"));
  gint data_type_bytes = channel_value_info->bits_per_channel / 8;

  GeglStorage * storage = g_object_new(GEGL_TYPE_COMPONENT_STORAGE,
                                       "data_type_bytes", data_type_bytes,
                                       "num_bands", color_model->num_channels,  
                                       "width", width,
                                       "height", height,
                                       "num_banks", color_model->num_channels, 
                                       NULL);

  return storage;
}
