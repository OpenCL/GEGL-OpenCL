#include "gegl-component-color-model.h"
#include "gegl-color-space.h"
#include "gegl-channel-space.h"
#include "gegl-component-storage.h"
#include "gegl-object.h"

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
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglColorModel *color_model = GEGL_COLOR_MODEL(gobject);
  gint num_colors = gegl_color_space_num_components(color_model->color_space);
  gint num_channels = num_colors;

  color_model->num_colors = num_colors;

  if(color_model->has_alpha)
    {
      color_model->alpha_channel = num_channels; 
      num_channels++;
    }

  if(color_model->has_z)
    {
      color_model->z_channel = num_channels;
      num_channels++;
    }

  color_model->num_channels = num_channels;

  color_model->bits_per_channel = g_new(gint, color_model->num_channels);

  for(i = 0; i < num_colors; i++)
    color_model->bits_per_channel[i] = gegl_channel_space_bits(color_model->channel_space);

  if(color_model->has_alpha)
    color_model->bits_per_channel[color_model->alpha_channel] = 
        gegl_channel_space_bits(color_model->channel_space);

  if(color_model->has_z)
    color_model->bits_per_channel[color_model->z_channel] = sizeof(gfloat);

  color_model->bits_per_pixel = 0;
  for(i = 0; i < color_model->num_channels; i++)
    color_model->bits_per_pixel += color_model->bits_per_channel[i];

  {
    gchar * color_space_name =  gegl_color_space_name(color_model->color_space);
    gchar * channel_space_name =  gegl_channel_space_name(color_model->channel_space);
    GString * name = g_string_new(color_space_name);

    if(color_model->has_alpha)
      name = g_string_append(name, "a");

    if(color_model->has_z)
      name = g_string_append(name, "z");

    name = g_string_append(name, "-");
    name = g_string_append(name, channel_space_name);

    color_model->name = g_strdup(name->str);
    g_string_free(name, TRUE);
  }

  color_model->channel_names = g_new(gchar*, color_model->num_channels);
  for(i = 0; i < color_model->num_colors; i++) 
    {
      gchar * channel_name = gegl_color_space_component_name(color_model->color_space, i);
      color_model->channel_names[i] = g_strdup(channel_name);
    }
    
  if(color_model->has_alpha)
    color_model->channel_names[color_model->alpha_channel] = g_strdup("alpha"); 

  if(color_model->has_z)
    color_model->channel_names[color_model->z_channel] = g_strdup("z"); 

  return gobject;
}

static GeglStorage * 
create_storage (GeglColorModel * color_model, 
                gint width, 
                gint height)
{
  GeglChannelSpace *channel_space = gegl_color_model_channel_space(color_model);
  gint data_type_bytes = gegl_channel_space_bits(channel_space) / 8;


  GeglStorage * storage = g_object_new(GEGL_TYPE_COMPONENT_STORAGE,
                                       "data_type_bytes", data_type_bytes,
                                       "num_bands", color_model->num_channels,  
                                       "width", width,
                                       "height", height,
                                       "num_banks", color_model->num_channels, 
                                       NULL);

  return storage;
}
