#include "gegl-tile.h"
#include "gegl-object.h"
#include "gegl-storage.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_STORAGE,
  PROP_LAST 
};

static void class_init (GeglTileClass * klass);
static void init (GeglTile *self, GeglTileClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_tile_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglTileClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglTile),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglTile", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglTileClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x",
                                                      "x coordinate for GeglTile",
                                                      "GeglTile x",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                      "y coordinate for GeglTile",
                                                      "GeglTile y",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                      "Width",
                                                      "GeglTile width",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                      "Height",
                                                      "GeglTile height",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_STORAGE,
                                   g_param_spec_object ("storage",
                                                        "Storage",
                                                        "The GeglTile's storage",
                                                         GEGL_TYPE_STORAGE,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglTile * self, 
      GeglTileClass * klass)
{
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglTile *self = GEGL_TILE(gobject);

  self->buffer = gegl_storage_create_buffer(self->storage);

  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglTile *self = GEGL_TILE (gobject);

  g_object_unref(self->buffer);
  g_object_unref(self->storage);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTile *self = GEGL_TILE (gobject);

  switch (prop_id)
  {
    case PROP_X:
      self->area.x = g_value_get_int(value);
      break;
    case PROP_Y:
      self->area.y = g_value_get_int(value);
      break;
    case PROP_WIDTH:
      self->area.w = g_value_get_int(value);
      break;
    case PROP_HEIGHT:
      self->area.h = g_value_get_int(value);
      break;
    case PROP_STORAGE:
      self->storage = (GeglStorage*) g_object_ref (g_value_get_object(value));
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
  GeglTile *self = GEGL_TILE (gobject);

  switch (prop_id)
  {
    case PROP_X:
      g_value_set_int(value, gegl_tile_get_x(self));
      break;
    case PROP_Y:
      g_value_set_int(value, gegl_tile_get_y(self));
      break;
    case PROP_WIDTH:
      g_value_set_int(value, gegl_tile_get_width(self));
      break;
    case PROP_HEIGHT:
      g_value_set_int(value, gegl_tile_get_height(self));
      break;
    case PROP_STORAGE:
      g_value_set_object (value, G_OBJECT(gegl_tile_get_storage(self)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_tile_get_buffer:
 * @self: a #GeglTile
 *
 * Gets the data buffer.  
 *
 * Returns: the #GeglBuffer for the tile. 
 **/
GeglBuffer * 
gegl_tile_get_buffer (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE (self), NULL);
   
  return self->buffer;
}

/**
 * gegl_tile_get_area:
 * @self: a #GeglTile
 * @area: rect to pass back.
 *
 * Gets the area of image space this tile represents.
 *
 **/
void 
gegl_tile_get_area (GeglTile * self, 
                    GeglRect * area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  gegl_rect_copy(area,&self->area);
}

gint  
gegl_tile_get_x (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_TILE (self), 0);

  return self->area.x;
}

gint  
gegl_tile_get_y (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_TILE (self), 0);

  return self->area.y;
}

gint  
gegl_tile_get_width (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_TILE (self), 0);

  return self->area.w;
}

gint  
gegl_tile_get_height (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_TILE (self), 0);

  return self->area.h;
}

/**
 * gegl_tile_get_storage:
 * @self: a #GeglTile
 *
 * Gets the storage of the tile.
 *
 * Returns: the #GeglStorage of the tile. 
 **/
GeglStorage * 
gegl_tile_get_storage (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE (self), NULL);

  return self->storage;
}

gpointer * 
gegl_tile_data_pointers (GeglTile * self, 
                         gint x, 
                         gint y)
{
   g_return_val_if_fail (self != NULL, NULL);
   g_return_val_if_fail (GEGL_IS_TILE (self), NULL);

   {
      gint i;
      gint x0 = x - self->area.x;
      gint y0 = y - self->area.y;

      gint num_channels = gegl_storage_num_bands(self->storage);
      gint bytes_per_channel = gegl_storage_data_type_bytes(self->storage);
      gint row_bytes = self->area.w * bytes_per_channel;
      gpointer *data_banks = gegl_buffer_banks_data(self->buffer);
      gpointer *data_pointers = g_new(gpointer, num_channels);

      /* Update the data pointers to point to (x,y) position */
      for(i=0; i < num_channels; i++)
        {
          data_pointers[i] = (gpointer)((guchar*)data_banks[i] + 
                                        y0 * row_bytes + 
                                        x0 * bytes_per_channel);
        }

      return data_pointers;
   }
}
