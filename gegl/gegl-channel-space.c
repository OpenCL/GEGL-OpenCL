#include "gegl-channel-space.h"
#include "gegl-object.h"

static void class_init (GeglChannelSpaceClass * klass);
static void init (GeglChannelSpace * self, GeglChannelSpaceClass * klass);
static void finalize(GObject *gobject);

static gpointer parent_class = NULL;
static GHashTable * channel_space_instances = NULL;

gboolean
gegl_channel_space_register(GeglChannelSpace *channel_space)
{
  g_return_val_if_fail (channel_space != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_CHANNEL_SPACE (channel_space), FALSE);

  {
    GeglChannelSpace * cs;

    if(!channel_space_instances)
      {
        g_print("ChannelSpace class not initialized\n");
        return FALSE;
      }

    /* Get the singleton ref for this color model. */
    g_object_ref(channel_space);

    g_hash_table_insert(channel_space_instances, 
                        (gpointer)gegl_channel_space_name(channel_space), 
                        (gpointer)channel_space);

    cs = g_hash_table_lookup(channel_space_instances, 
                             (gpointer)gegl_channel_space_name(channel_space));

    if(cs != channel_space)
      return FALSE;
  }


  return TRUE;
} 

GeglChannelSpace *
gegl_channel_space_instance(gchar *channel_space_name)
{
  GeglChannelSpace * channel_space = g_hash_table_lookup(channel_space_instances, 
                                                   (gpointer)channel_space_name);
  if(!channel_space)
    {
      g_warning("%s not registered as ChannelSpace.\n", channel_space_name);
      return NULL;
    }

  return channel_space;
} 

GType
gegl_channel_space_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglChannelSpaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglChannelSpace),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglChannelSpace", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglChannelSpaceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->convert_to_float = NULL;
  klass->convert_from_float = NULL;
  klass->convert_value_to_float = NULL;
  klass->convert_value_from_float = NULL;

  channel_space_instances = g_hash_table_new(g_str_hash,g_str_equal);

  return;
}

static void 
init (GeglChannelSpace * self, 
      GeglChannelSpaceClass * klass)
{
  self->channel_space_type = GEGL_CHANNEL_SPACE_NONE;
  self->name = NULL;
  self->bits = 0;
  self->is_channel_data = TRUE;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglChannelSpace *self = GEGL_CHANNEL_SPACE(gobject);

  if(self->name)
    g_free(self->name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_channel_space_channel_space_type:
 * @self: a #GeglChannelSpace
 *
 * Gets the type of the color space. (eg FLOAT, U8, etc)
 *
 * Returns: the #GeglChannelSpaceType of this color model.
 **/
GeglChannelSpaceType 
gegl_channel_space_channel_space_type (GeglChannelSpace * self)
{
  g_return_val_if_fail (self != NULL, GEGL_CHANNEL_SPACE_NONE);
  g_return_val_if_fail (GEGL_IS_CHANNEL_SPACE (self), GEGL_CHANNEL_SPACE_NONE);

  return self->channel_space_type; 
}

/**
 * gegl_channel_space_name:
 * @self: a #GeglChannelSpace
 *
 * Gets the name of the data space.
 *
 * Returns: the name of the data space.
 **/
gchar *
gegl_channel_space_name (GeglChannelSpace * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_CHANNEL_SPACE (self), NULL);

  return self->name; 
}

/**
 * gegl_channel_space_bits:
 * @self: a #GeglChannelSpace
 *
 * Gets the bits for the data space type.
 *
 * Returns: the bits for the data space type.
 **/
gint
gegl_channel_space_bits (GeglChannelSpace * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_CHANNEL_SPACE (self), 0);

  return self->bits; 
}

/**
 * gegl_channel_space_is_channel_data:
 * @self: a #GeglChannelSpace
 *
 * Tests if the data space corresponds to channel data. 
 * (ComponentColorModels do have this, INDEXED color models dont)
 *
 * Returns: TRUE if the data space represents channel data.
 **/
gboolean
gegl_channel_space_is_channel_data (GeglChannelSpace * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_CHANNEL_SPACE (self), FALSE);

  return self->is_channel_data; 
}

/**
 * gegl_channel_space_convert_to_float:
 * @self: a #GeglChannelSpace
 * @dest: dest float data.
 * @src: src data.
 * @num: number to process.
 *
 * Converts from src data space to float data.
 *
 **/
void 
gegl_channel_space_convert_to_float (GeglChannelSpace * self, 
                                     gfloat * dest, 
                                     gpointer src, 
                                     gint num)
{
   GeglChannelSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_CHANNEL_SPACE (self));
   klass = GEGL_CHANNEL_SPACE_GET_CLASS(self);

   if(klass->convert_to_float)
      (*klass->convert_to_float)(self,dest,src,num);
}

void 
gegl_channel_space_convert_value_to_float (GeglChannelSpace * self, 
                                           GValue * dest, 
                                           GValue * src)
{
   GeglChannelSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_CHANNEL_SPACE (self));
   klass = GEGL_CHANNEL_SPACE_GET_CLASS(self);

   if(klass->convert_value_to_float)
      (*klass->convert_value_to_float)(self,dest,src);
}

/**
 * gegl_channel_space_convert_from_float:
 * @self: a #GeglChannelSpace
 * @dest: dest data.
 * @src: src float data.
 * @num: number to process.
 *
 * Converts float data to dest data space.
 *
 **/
void 
gegl_channel_space_convert_from_float (GeglChannelSpace * self, 
                                       gpointer dest, 
                                       gfloat * src, 
                                       gint num)
{
   GeglChannelSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_CHANNEL_SPACE (self));
   klass = GEGL_CHANNEL_SPACE_GET_CLASS(self);

   if(klass->convert_from_float)
      (*klass->convert_from_float)(self,dest,src,num);
}

void 
gegl_channel_space_convert_value_from_float (GeglChannelSpace * self, 
                                             GValue * dest, 
                                             GValue * src)
{
   GeglChannelSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_CHANNEL_SPACE (self));
   klass = GEGL_CHANNEL_SPACE_GET_CLASS(self);

   if(klass->convert_from_float)
      (*klass->convert_value_from_float)(self,dest,src);
}
