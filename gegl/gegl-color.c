#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-color.h"
#include "gegl-object.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_COLOR_MODEL,
  PROP_LAST 
};

static void class_init (GeglColorClass * klass);
static void init (GeglColor * self, GeglColorClass * klass);
static void finalize (GObject *gobject);
static void set_color_model (GeglColor * self, GeglColorModel *color_model);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_color_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, "GeglColor", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglColorClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   parent_class = g_type_class_peek_parent (klass);

   gobject_class->finalize = finalize;
   gobject_class->set_property = set_property;
   gobject_class->get_property = get_property;

   g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                    g_param_spec_object ("colormodel",
                                                         "ColorModel",
                                                         "The GeglColor's colormodel",
                                                          GEGL_TYPE_COLOR_MODEL,
                                                          G_PARAM_CONSTRUCT |
                                                          G_PARAM_READWRITE));

   return;
}

static void 
init (GeglColor * self, 
      GeglColorClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
   GeglColor *self = GEGL_COLOR (gobject);

   /* Dispose of channels values array*/
   g_free(self->channel_values);

   /* Unref the color model */
   g_object_unref(self->color_model);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColor *self = GEGL_COLOR (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
        if(!GEGL_OBJECT(gobject)->constructed)
          set_color_model (self,GEGL_COLOR_MODEL(g_value_get_object(value)));
        else
          g_message("Cant set colormodel after construction\n");
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
  GeglColor *self = GEGL_COLOR (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
        g_value_set_object (value, G_OBJECT(gegl_color_get_color_model(self)));
      break;
    default:
      break;
  }
}


/**
 * gegl_color_get_channel_values:
 * @self: a #GeglColor.
 *
 * Gets the array of channel values.
 *
 * Returns: a #GeglChannelValue array.
 **/
GeglChannelValue * 
gegl_color_get_channel_values (GeglColor * self)
{
  g_return_val_if_fail (self != NULL, (GeglChannelValue * )0);
  g_return_val_if_fail (GEGL_IS_COLOR (self), (GeglChannelValue * )0);
   
  return self->channel_values;
}

/**
 * gegl_color_get_color_model:
 * @self: a #GeglColor.
 *
 * Gets the color model of the color.
 *
 * Returns: a #GeglColorModel. 
 **/
GeglColorModel * 
gegl_color_get_color_model (GeglColor * self)
{
  g_return_val_if_fail (self != NULL, (GeglColorModel * )0);
  g_return_val_if_fail (GEGL_IS_COLOR (self), (GeglColorModel * )0);
   
  return self->color_model;
}

static void
set_color_model (GeglColor * self, 
                 GeglColorModel *color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (color_model != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (GEGL_IS_COLOR_MODEL (color_model));
   
  self->color_model = color_model;
  g_object_ref(color_model);
  self->num_channels = gegl_color_model_num_channels(color_model);
  self->channel_values = g_new (GeglChannelValue, self->num_channels);
}

/**
 * gegl_color_get_num_channels:
 * @self: a #GeglColor.
 *
 * Gets the number of channels in the color. 
 *
 * Returns: number of channels in the color. 
 **/
int 
gegl_color_get_num_channels (GeglColor * self)
{
  g_return_val_if_fail (self != NULL, (int )0);
  g_return_val_if_fail (GEGL_IS_COLOR (self), (int )0);
   
  return self->num_channels;
}

/**
 * gegl_color_set_channel_values:
 * @self: a #GeglColor.
 * @channel_values: a #GeglChannelValue array.
 *
 * Copy @channel_values into the color's #GeglChannelValue array.  
 *
 **/
void 
gegl_color_set_channel_values (GeglColor * self, 
                               GeglChannelValue * channel_values)
{
  gint i;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));
   
  i = self->num_channels;
  while (i--)
    self->channel_values[i] = channel_values[i]; 
}

/**
 * gegl_color_set:
 * @self: a #GeglColor.
 * @color: set it to this #GeglColor.
 *
 * Set the channel values of @self to the channel values of @color.
 *
 **/
void 
gegl_color_set (GeglColor * self, 
                GeglColor * color)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));
   
  {
    GeglChannelValue *channel_values = gegl_color_get_channel_values(color); 
    gegl_color_set_channel_values (self, channel_values);
  }
}

/**
 * gegl_color_set_constant:
 * @self: a #GeglColor.
 * @constant: a #GeglColorConstant.
 *
 * Set the channel values for @self based on @constant.
 *
 **/
void 
gegl_color_set_constant (GeglColor * self, 
                         GeglColorConstant constant)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));
   
  gegl_color_model_set_color(self->color_model, self, constant);
}
