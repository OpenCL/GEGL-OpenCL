#include "gegl-storage.h"

enum
{
  PROP_0, 
  PROP_DATA_TYPE_BYTES,
  PROP_NUM_BANDS,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_LAST 
};

static void class_init (GeglStorageClass * klass);
static void init (GeglStorage * self, GeglStorageClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_storage_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglStorageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglStorage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglStorage", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglStorageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->create_buffer = NULL;

  g_object_class_install_property (gobject_class, PROP_DATA_TYPE_BYTES,
                                   g_param_spec_int ("data_type_bytes",
                                                      "Data type bytes",
                                                      "GeglStorage data type bytes",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_BANDS,
                                   g_param_spec_int ("num_bands",
                                                      "Number of Bands ",
                                                      "GeglStorage number of bands",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT | 
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                      "Width",
                                                      "Width of this storage",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT | 
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                      "Height",
                                                      "Height of this storage",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT | 
                                                      G_PARAM_READWRITE));
}

static void 
init (GeglStorage * self, 
      GeglStorageClass * klass)
{
  self->data_type_bytes = 0;
  self->num_bands = 0;
  self->width = 0;
  self->height = 0;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglStorage *self = GEGL_STORAGE (gobject);

  switch (prop_id)
  {
    case PROP_DATA_TYPE_BYTES:
      self->data_type_bytes = g_value_get_int (value);
      break;
    case PROP_NUM_BANDS:
      self->num_bands = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      self->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
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
  GeglStorage *self = GEGL_STORAGE (gobject);

  switch (prop_id)
  {
    case PROP_DATA_TYPE_BYTES:
      g_value_set_int (value, gegl_storage_data_type_bytes(self));
      break;
    case PROP_NUM_BANDS:
      g_value_set_int (value, gegl_storage_num_bands(self));
      break;
    case PROP_WIDTH:
      g_value_set_int (value, gegl_storage_width(self));
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, gegl_storage_height(self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}


/**
 * gegl_storage_data_type_bytes:
 * @self: a #GeglStorage
 *
 * Gets the bytes for the data type.
 *
 * Returns: number of bytes for data type. 
 **/
gint 
gegl_storage_data_type_bytes (GeglStorage * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_STORAGE (self), 0);

  return self->data_type_bytes;
}

/**
 * gegl_storage_num_bands:
 * @self: a #GeglStorage
 *
 * Gets the number of bands of data.
 *
 * Returns: number of bands of data. 
 **/
gint 
gegl_storage_num_bands (GeglStorage * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_STORAGE (self), 0);

  return self->num_bands;
}

/**
 * gegl_storage_width:
 * @self: a #GeglStorage
 *
 * Gets the width of this storage.
 *
 * Returns: width. 
 **/
gint 
gegl_storage_width(GeglStorage * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_STORAGE (self), 0);

  return self->width;
}

/**
 * gegl_storage_height:
 * @self: a #GeglStorage
 *
 * Gets the height of this storage.
 *
 * Returns: height. 
 **/
gint 
gegl_storage_height(GeglStorage * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_STORAGE (self), 0);

  return self->height;
}

GeglBuffer* 
gegl_storage_create_buffer  (GeglStorage * self)
{
   GeglStorageClass *klass;
   g_return_val_if_fail (self != NULL, NULL);
   g_return_val_if_fail (GEGL_IS_STORAGE (self), NULL);
   klass = GEGL_STORAGE_GET_CLASS(self);

   if(klass->create_buffer)
      return (*klass->create_buffer)(self);
   else
      return NULL;
}
