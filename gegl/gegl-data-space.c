#include "gegl-data-space.h"
#include "gegl-object.h"

static void class_init (GeglDataSpaceClass * klass);
static void init (GeglDataSpace * self, GeglDataSpaceClass * klass);
static void finalize(GObject *gobject);

static gpointer parent_class = NULL;
static GHashTable * data_space_instances = NULL;

gboolean
gegl_data_space_register(GeglDataSpace *data_space)
{
  g_return_val_if_fail (data_space != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_DATA_SPACE (data_space), FALSE);

  {
    GeglDataSpace * cs;

    if(!data_space_instances)
      {
        g_print("DataSpace class not initialized\n");
        return FALSE;
      }

    /* Get the singleton ref for this color model. */
    g_object_ref(data_space);

    g_hash_table_insert(data_space_instances, 
                        (gpointer)gegl_data_space_name(data_space), 
                        (gpointer)data_space);

    cs = g_hash_table_lookup(data_space_instances, 
                             (gpointer)gegl_data_space_name(data_space));

    if(cs != data_space)
      return FALSE;
  }


  return TRUE;
} 

GeglDataSpace *
gegl_data_space_instance(gchar *data_space_name)
{
  GeglDataSpace * data_space = g_hash_table_lookup(data_space_instances, 
                                                   (gpointer)data_space_name);
  if(!data_space)
    {
      g_warning("%s not registered as DataSpace.\n", data_space_name);
      return NULL;
    }

  return data_space;
} 

GType
gegl_data_space_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDataSpaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDataSpace),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglDataSpace", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglDataSpaceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->convert_to_float = NULL;
  klass->convert_from_float = NULL;
  klass->convert_value_to_float = NULL;
  klass->convert_value_from_float = NULL;

  data_space_instances = g_hash_table_new(g_str_hash,g_str_equal);

  return;
}

static void 
init (GeglDataSpace * self, 
      GeglDataSpaceClass * klass)
{
  self->data_space_type = GEGL_DATA_SPACE_NONE;
  self->name = NULL;
  self->bits = 0;
  self->is_channel_data = TRUE;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglDataSpace *self = GEGL_DATA_SPACE(gobject);

  if(self->name)
    g_free(self->name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_data_space_data_space_type:
 * @self: a #GeglDataSpace
 *
 * Gets the type of the color space. (eg FLOAT, U8, etc)
 *
 * Returns: the #GeglDataSpaceType of this color model.
 **/
GeglDataSpaceType 
gegl_data_space_data_space_type (GeglDataSpace * self)
{
  g_return_val_if_fail (self != NULL, GEGL_DATA_SPACE_NONE);
  g_return_val_if_fail (GEGL_IS_DATA_SPACE (self), GEGL_DATA_SPACE_NONE);

  return self->data_space_type; 
}

/**
 * gegl_data_space_name:
 * @self: a #GeglDataSpace
 *
 * Gets the name of the data space.
 *
 * Returns: the name of the data space.
 **/
gchar *
gegl_data_space_name (GeglDataSpace * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_DATA_SPACE (self), NULL);

  return self->name; 
}

/**
 * gegl_data_space_bits:
 * @self: a #GeglDataSpace
 *
 * Gets the bits for the data space type.
 *
 * Returns: the bits for the data space type.
 **/
gint
gegl_data_space_bits (GeglDataSpace * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_DATA_SPACE (self), 0);

  return self->bits; 
}

/**
 * gegl_data_space_is_channel_data:
 * @self: a #GeglDataSpace
 *
 * Tests if the data space corresponds to channel data. 
 * (ComponentColorModels do have this, INDEXED color models dont)
 *
 * Returns: TRUE if the data space represents channel data.
 **/
gboolean
gegl_data_space_is_channel_data (GeglDataSpace * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_DATA_SPACE (self), FALSE);

  return self->is_channel_data; 
}

/**
 * gegl_data_space_convert_to_float:
 * @self: a #GeglDataSpace
 * @dest: dest float data.
 * @src: src data.
 * @num: number to process.
 *
 * Converts from src data space to float data.
 *
 **/
void 
gegl_data_space_convert_to_float (GeglDataSpace * self, 
                                  gfloat * dest, 
                                  gpointer src, 
                                  gint num)
{
   GeglDataSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_DATA_SPACE (self));
   klass = GEGL_DATA_SPACE_GET_CLASS(self);

   if(klass->convert_to_float)
      (*klass->convert_to_float)(self,dest,src,num);
}

void 
gegl_data_space_convert_value_to_float (GeglDataSpace * self, 
                                        GValue * dest, 
                                        GValue * src)
{
   GeglDataSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_DATA_SPACE (self));
   klass = GEGL_DATA_SPACE_GET_CLASS(self);

   if(klass->convert_value_to_float)
      (*klass->convert_value_to_float)(self,dest,src);
}

/**
 * gegl_data_space_convert_from_float:
 * @self: a #GeglDataSpace
 * @dest: dest data.
 * @src: src float data.
 * @num: number to process.
 *
 * Converts float data to dest data space.
 *
 **/
void 
gegl_data_space_convert_from_float (GeglDataSpace * self, 
                                    gpointer dest, 
                                    gfloat * src, 
                                    gint num)
{
   GeglDataSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_DATA_SPACE (self));
   klass = GEGL_DATA_SPACE_GET_CLASS(self);

   if(klass->convert_from_float)
      (*klass->convert_from_float)(self,dest,src,num);
}

void 
gegl_data_space_convert_value_from_float (GeglDataSpace * self, 
                                          GValue * dest, 
                                          GValue * src)
{
   GeglDataSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_DATA_SPACE (self));
   klass = GEGL_DATA_SPACE_GET_CLASS(self);

   if(klass->convert_from_float)
      (*klass->convert_value_from_float)(self,dest,src);
}
