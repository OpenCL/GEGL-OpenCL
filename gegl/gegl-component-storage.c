#include "gegl-component-storage.h"
#include "gegl-buffer.h"

enum
{
  PROP_0, 
  PROP_NUM_BANKS,
  PROP_LAST 
};

static void class_init (GeglComponentStorageClass * klass);
static void init (GeglComponentStorage * self, GeglComponentStorageClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglBuffer * create_buffer(GeglStorage * storage);

static gpointer parent_class = NULL;

GType
gegl_component_storage_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglComponentStorageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglComponentStorage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_STORAGE , 
                                     "GeglComponentStorage", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglComponentStorageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglStorageClass *storage_class = GEGL_STORAGE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor = constructor;

  storage_class->create_buffer = create_buffer;

  g_object_class_install_property (gobject_class, PROP_NUM_BANKS,
                                   g_param_spec_int ("num_banks",
                                                      "Number of Banks ",
                                                      "GeglComponentStorage number of banks",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT | 
                                                      G_PARAM_READWRITE));

}

static void 
init (GeglComponentStorage * self, 
      GeglComponentStorageClass * klass)
{
  self->pixel_stride_bytes = 0;
  self->row_stride_bytes = 0;
  self->num_banks = 0;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglComponentStorage *self = GEGL_COMPONENT_STORAGE(gobject);
  GeglStorage * storage = GEGL_STORAGE(gobject);

  self->pixel_stride_bytes = storage->data_type_bytes;
  self->row_stride_bytes = storage->data_type_bytes * storage->width;

  return gobject;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglComponentStorage *self = GEGL_COMPONENT_STORAGE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_BANKS:
      self->num_banks = g_value_get_int (value);
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
  GeglComponentStorage *self = GEGL_COMPONENT_STORAGE (gobject);

  switch (prop_id)
  {
    case PROP_NUM_BANKS:
      g_value_set_int (value, gegl_component_storage_num_banks(self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}


/**
 * gegl_component_storage_num_banks:
 * @self: a #GeglComponentStorage
 *
 * Gets the number of banks for the data buffer.
 *
 * Returns: number of banks for data buffer. 
 **/
gint 
gegl_component_storage_num_banks (GeglComponentStorage * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_COMPONENT_STORAGE (self), 0);

  return self->num_banks;
}

static GeglBuffer* 
create_buffer  (GeglStorage * storage)
{
  GeglComponentStorage * self = GEGL_COMPONENT_STORAGE(storage);
  gint bank_bytes = storage->width * storage->height * storage->data_type_bytes;

  GeglBuffer * buffer = g_object_new(GEGL_TYPE_BUFFER,
                                              "num_banks", self->num_banks,
                                              "bytes_per_bank", bank_bytes,
                                              NULL);

  return buffer;
}
