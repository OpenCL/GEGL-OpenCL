#include "gegl-tile-iterator.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-utils.h"
#include "gegl-tile.h"

enum
{
  PROP_0, 
  PROP_TILE,
  PROP_AREA,
  PROP_LAST 
};

static void class_init (GeglTileIteratorClass * klass);
static void init (GeglTileIterator * self, GeglTileIteratorClass * klass);
static void finalize (GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_tile_iterator_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglTileIteratorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglTileIterator),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , "GeglTileIterator", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglTileIteratorClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->finalize = finalize;
   gobject_class->set_property = set_property;
   gobject_class->get_property = get_property;

   g_object_class_install_property (gobject_class, PROP_TILE,
                                   g_param_spec_object ("tile",
                                                        "Tile",
                                                        "The GeglTileIterator's tile",
                                                         GEGL_TYPE_TILE,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

   g_object_class_install_property (gobject_class, PROP_AREA,
                                    g_param_spec_pointer ("area",
                                                          "Area",
                                                          "The (sub)rect area of the iterator's tile to process",
                                                          G_PARAM_CONSTRUCT |
                                                          G_PARAM_READWRITE));

   return;
}

static void 
init (GeglTileIterator * self, 
      GeglTileIteratorClass * klass)
{
  self->current = 0;
}

static void
finalize(GObject *gobject)
{
   GeglTileIterator *self = GEGL_TILE_ITERATOR (gobject);
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
  GeglTileIterator *self = GEGL_TILE_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_TILE:
        gegl_tile_iterator_set_tile (self, g_value_get_tile(value));
      break;
    case PROP_AREA:
        gegl_tile_iterator_set_rect (self, (GeglRect*)g_value_get_pointer (value));
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
  GeglTileIterator *self = GEGL_TILE_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_TILE:
      g_value_set_tile (value, gegl_tile_iterator_get_tile(self));
      break;
    case PROP_AREA:
      gegl_tile_iterator_get_rect (self, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      break;
  }
}

/**
 * gegl_tile_iterator_get_tile:
 * @self: a #GeglTileIterator
 *
 * Gets the #GeglTile this iterator uses.
 *
 * Returns: a #GeglTile this iterator uses. 
 **/
GeglTile * 
gegl_tile_iterator_get_tile (GeglTileIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), NULL);
   
  return self->tile;
}

void 
gegl_tile_iterator_set_tile (GeglTileIterator * self, 
                             GeglTile * tile)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));

  if(self->tile)
    g_object_unref(self->tile);
   
  self->tile = tile;

  /* Ref the tile */
  if(tile)
    g_object_ref(tile);
}

void 
gegl_tile_iterator_get_rect (GeglTileIterator * self, 
                             GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));
  g_return_if_fail (rect != NULL);

  gegl_rect_copy(rect,&self->rect);
}

void 
gegl_tile_iterator_set_rect (GeglTileIterator * self, GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));
  g_return_if_fail (rect!= NULL);

  gegl_rect_copy(&self->rect, rect);
}

/**
 * gegl_tile_iterator_get_color_model:
 * @self: a #GeglTileIterator.
 *
 * Gets the color model of the tile.
 *
 * Returns: the #GeglColorModel of the tile. 
 **/
GeglColorModel * 
gegl_tile_iterator_get_color_model (GeglTileIterator * self)
{
  g_return_val_if_fail (self != NULL, (GeglColorModel * )0);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), (GeglColorModel * )0);

  return gegl_tile_get_color_model(self->tile);
}

gint            
gegl_tile_iterator_get_num_colors(GeglTileIterator * self)
{
  GeglColorModel *color_model;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), -1);

  color_model = gegl_tile_get_color_model(self->tile);
  return gegl_color_model_num_colors(color_model);
}

/**
 * gegl_tile_iterator_first:
 * @self: a #GeglTileIterator.
 *
 * Makes the iterator point to the first scanline.
 *
 **/
void 
gegl_tile_iterator_first (GeglTileIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));
  
  self->current = 0;
}

/**
 * gegl_tile_iterator_next:
 * @self: a #GeglTileIterator.
 *
 * Makes iterator point to the next scanline. 
 *
 **/
void 
gegl_tile_iterator_next (GeglTileIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));
   
  self->current++;
}

/**
 * gegl_tile_iterator_is_done:
 * @self: a #GeglTileIterator.
 *
 * Checks if there are more scanlines. 
 *
 * Returns: TRUE if there are more scanlines.
 **/
gboolean 
gegl_tile_iterator_is_done (GeglTileIterator * self)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), (gboolean )0);
   
  if (self->current < self->rect.h) 
    return FALSE;
  else
    return TRUE;
}

/**
 * gegl_tile_iterator_color_channels:
 * @self: a #GeglTileIterator.
 *
 * Gets an array of pointers to channel data. Should be freed.
 * If any channels are masked, they are passed back as NULL.
 *
 * Returns: An array of pointers to channel data.
 **/
gpointer *
gegl_tile_iterator_color_channels(GeglTileIterator *self)
{
  gpointer * data_pointers; 
  gpointer * masked_data_pointers; 
  GeglColorModel *color_model;
  gint i;
  gint num_color_chans;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), NULL);

  data_pointers = gegl_tile_data_pointers(self->tile,
                                          self->rect.x, 
                                          self->rect.y + self->current);  

  color_model = gegl_tile_get_color_model(self->tile);
  num_color_chans = gegl_color_model_num_colors(color_model);

  masked_data_pointers = g_new(gpointer, num_color_chans); 

  for(i = 0; i < num_color_chans; i++)
    masked_data_pointers[i] = data_pointers[i];

  g_free(data_pointers);

  return masked_data_pointers;
}

/**
 * gegl_tile_iterator_alpha_channel:
 * @self: a #GeglTileIterator.
 *
 * Gets a pointer to the alpha channel at current iterator location.
 * If the alpha channel is masked or non-existent, this is NULL.
 *
 * Returns: Pointer to alpha channel data.
 **/
gpointer
gegl_tile_iterator_alpha_channel(GeglTileIterator *self)
{
  gint alpha_chan;
  gpointer *data_pointers;
  GeglColorModel *color_model;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_ITERATOR (self), NULL);

  data_pointers = gegl_tile_data_pointers(self->tile,
                                          self->rect.x, 
                                          self->rect.y + self->current);  

  color_model = gegl_tile_get_color_model(self->tile);
  alpha_chan = gegl_color_model_alpha_channel(color_model);

  if(alpha_chan >= 0)
    return data_pointers[alpha_chan];
  else
    return NULL;
}
                                                

/**
 * gegl_tile_iterator_get_current:
 * @self: a #GeglTileIterator.
 * @data_pointers: array of data pointers.
 *
 * Makes @data_pointers point to the current scanline data. 
 *
 **/
void 
gegl_tile_iterator_get_current (GeglTileIterator * self, 
                                gpointer * data_pointers)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_ITERATOR (self));
   
  /* Just pass the data pointers through to the tile data*/
  gegl_tile_get_data_at(self->tile, 
                        data_pointers, 
                        self->rect.x, 
                        self->rect.y + self->current);  

}
