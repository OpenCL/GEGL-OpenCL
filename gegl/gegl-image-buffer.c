#include "gegl-image-buffer.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_COLOR_MODEL,
  PROP_LAST 
};

static void class_init (GeglImageBufferClass * klass);
static void init (GeglImageBuffer *self, GeglImageBufferClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_image_buffer_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageBufferClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglImageBuffer),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglImageBuffer", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglImageBufferClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                   g_param_spec_object ("color_model",
                                                        "ColorModel",
                                                        "The GeglImageBuffer's color model",
                                                         GEGL_TYPE_COLOR_MODEL,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglImageBuffer * self, 
      GeglImageBufferClass * klass)
{
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglImageBuffer *self = GEGL_IMAGE_BUFFER (gobject);

  if(self->tile)
    g_object_unref(self->tile);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImageBuffer *self = GEGL_IMAGE_BUFFER (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      gegl_image_buffer_set_color_model(self, (GeglColorModel*)g_value_get_object(value));
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
  GeglImageBuffer *self = GEGL_IMAGE_BUFFER (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      g_value_set_object (value, (GObject*)gegl_image_buffer_get_color_model(self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

GeglTile *    
gegl_image_buffer_get_tile (GeglImageBuffer * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER (self), NULL);

  return self->tile;
}

void            
gegl_image_buffer_set_tile (GeglImageBuffer * self,
                            GeglTile *tile)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER (self));

  if(tile)
    g_object_ref(tile);

  if(self->tile)
    g_object_unref(self->tile);

  self->tile = tile;
}

GeglColorModel*   
gegl_image_buffer_get_color_model (GeglImageBuffer * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER (self), NULL);

  return self->color_model;
}

void 
gegl_image_buffer_set_color_model (GeglImageBuffer * self, 
                               GeglColorModel * color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER (self));

  self->color_model = color_model;
}

void
gegl_image_buffer_create_tile (GeglImageBuffer * self, 
                               GeglColorModel * color_model,
                               GeglRect * area)
{
  GeglStorage *storage;
  GeglTile * tile;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER(self));
  g_return_if_fail (color_model != NULL);
  g_return_if_fail (GEGL_IS_COLOR_MODEL(color_model));
  g_return_if_fail (area != NULL);

  gegl_image_buffer_set_color_model(self, color_model);

  storage = gegl_color_model_create_storage(self->color_model, area->w, area->h);
  tile = g_object_new (GEGL_TYPE_TILE, 
                         "x", area->x, 
                         "y", area->y, 
                         "width", area->w, 
                         "height", area->h,
                         "storage", storage,
                         NULL);  

  gegl_image_buffer_set_tile(self, tile);

  g_object_unref(storage);
  g_object_unref(tile);
}
