#include "gegl-tile.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-tile-mgr.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"


enum
{
  PROP_0, 
  PROP_AREA,
  PROP_COLOR_MODEL,
  PROP_LAST 
};

static void class_init (GeglTileClass * klass);
static void init (GeglTile *self, GeglTileClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void unalloc (GeglTile * self);

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
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, "GeglTile", &typeInfo, 0);
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

  g_object_class_install_property (gobject_class, PROP_AREA,
                                   g_param_spec_pointer ("area",
                                                        "Area",
                                                        "The GeglTile's area",
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                   g_param_spec_object ("colormodel",
                                                        "ColorModel",
                                                        "The GeglTile's colormodel",
                                                         GEGL_TYPE_COLOR_MODEL,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglTile * self, 
      GeglTileClass * klass)
{
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglTile *self = GEGL_TILE(gobject);

  if(!gegl_tile_alloc(self, &self->area, self->color_model))
    return FALSE;

  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglTile *self = GEGL_TILE (gobject);
  unalloc(self);

  if(self->color_model) 
    g_object_ref(self->color_model);

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
    case PROP_AREA:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          gegl_tile_set_area (self, (GeglRect*)g_value_get_pointer (value));
        }
      else
        g_message("Cant set area after construction\n");
      break;
    case PROP_COLOR_MODEL:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          GeglColorModel *color_model = GEGL_COLOR_MODEL(g_value_get_object(value)); 
          gegl_tile_set_color_model (self,color_model);
        }
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
  GeglTile *self = GEGL_TILE (gobject);

  switch (prop_id)
  {
    case PROP_AREA:
      gegl_tile_get_area (self, (GeglRect*)g_value_get_pointer (value));
      break;
    case PROP_COLOR_MODEL:
      g_value_set_object (value, G_OBJECT(gegl_tile_get_color_model(self)));
      break;
    default:
      break;
  }
}


/**
 * gegl_tile_get_buffer:
 * @self: a #GeglTile
 *
 * Gets a pointer to the buffer.  
 *
 * Returns: the #GeglBuffer for the tile. 
 **/
GeglBuffer * 
gegl_tile_get_buffer (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, (GeglBuffer * )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (GeglBuffer * )0);
   
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
gegl_tile_get_area (GeglTile * self, GeglRect * area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  gegl_rect_copy(area,&self->area);
}

void 
gegl_tile_set_area (GeglTile * self, GeglRect * area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));
 
  gegl_rect_copy(&self->area, area);
}

/**
 * gegl_tile_get_color_model:
 * @self: a #GeglTile
 *
 * Gets the color model of the tile.
 *
 * Returns: the #GeglColorModel of the tile. 
 **/
GeglColorModel * 
gegl_tile_get_color_model (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, (GeglColorModel * )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (GeglColorModel * )0);

  return self->color_model;
}

void
gegl_tile_set_color_model (GeglTile * self, 
                           GeglColorModel * color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));
  g_return_if_fail (color_model != NULL);
  g_return_if_fail (GEGL_IS_COLOR_MODEL (color_model));

  self->color_model = color_model;

  g_object_ref(self->color_model);
}

/**
 * gegl_tile_get_data:
 * @self: a #GeglTile
 * @data: data pointers to fill in.
 *
 * Makes the passed data pointers point to data in the tile's
 * #GeglDataBuffer.  These will point to data that represents the upper left
 * corner of the tile. ie (area.x,area.y).  
 *
 **/
void 
gegl_tile_get_data (GeglTile * self, gpointer * data)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_TILE (self));
   {
     gint i;
     gpointer * data_pointers = gegl_buffer_get_data_pointers(self->buffer);
     gint num_channels = gegl_color_model_num_channels(self->color_model);

      /* Copy the data pointers into the passed array */
      for(i=0; i < num_channels; i++)
        data[i] = data_pointers[i];
   }
}

/**
 * gegl_tile_get_data_at:
 * @self: a #GeglTile
 * @data: data pointers to fill in.
 * @x: x in [area.x, area.x + area.w - 1]
 * @y: y in [area.y, area.y + area.h - 1]
 *
 * Makes the passed data pointers point to data in the tile's #GeglDataBuffer
 * at (x,y). This is offset by  x - area.x and y - area.y from upper left
 * corner of the tile. 
 *
 **/
void 
gegl_tile_get_data_at (GeglTile * self, gpointer * data, gint x, gint y)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_TILE (self));
   {
      gint i;
      gint x0 = x - self->area.x;
      gint y0 = y - self->area.y;

      gint num_channels = gegl_color_model_num_channels(self->color_model);
      gint bytes_per_channel = gegl_color_model_bytes_per_channel(self->color_model);
      gint channel_row_bytes = self->area.w * bytes_per_channel;

      /*g_print("get_data_at: %d %d\n", x0,y0); */
      /* Get data pointers for beginning of tile. */
      gegl_tile_get_data(self,data);

      /* Update the data pointers to point to (x,y) position */
      for(i=0; i < num_channels; i++)
        {
          data[i] = (gpointer)((guchar*)data[i] + 
                                y0 * channel_row_bytes + 
                                x0 * bytes_per_channel);
        }
   }
}

static void 
unalloc (GeglTile * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  if (self->color_model)
    {
      g_object_unref(self->color_model);
      self->color_model = NULL;
    }

  if (self->buffer)
    {
      g_object_unref(self->buffer);
      self->buffer = NULL;
    }
}

gboolean 
gegl_tile_alloc (GeglTile * self, 
                 GeglRect * area, 
                 GeglColorModel * color_model)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (gboolean )0);

  {
    GeglTileMgr * tile_mgr = gegl_tile_mgr_instance();

    g_assert(tile_mgr != NULL);

    /* Get rid of any old data and color model. */
    unalloc(self);

    self->color_model = color_model;
    g_object_ref(color_model);
    gegl_rect_copy(&self->area, area); 

    self->buffer = gegl_tile_mgr_create_buffer(tile_mgr, self); 
                                                     
    g_object_unref(tile_mgr);

    if(!self->buffer)
      return FALSE;

    return TRUE;
  }
}

void 
gegl_tile_validate_data (GeglTile * self)
{
  gpointer * data_pointers = NULL;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  data_pointers = gegl_buffer_get_data_pointers(self->buffer);

  if(!data_pointers)
    gegl_buffer_alloc_data(self->buffer);
}
