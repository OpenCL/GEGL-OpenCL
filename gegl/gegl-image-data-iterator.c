#include "gegl-image-data-iterator.h"
#include "gegl-color-model.h"
#include "gegl-image-data.h"
#include "gegl-tile.h"

enum
{
  PROP_0, 
  PROP_IMAGE_DATA,
  PROP_AREA,
  PROP_LAST 
};

static void class_init (GeglImageDataIteratorClass * klass);
static void init (GeglImageDataIterator * self, GeglImageDataIteratorClass * klass);
static void finalize (GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_image_data_iterator_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageDataIteratorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageDataIterator),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglImageDataIterator", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglImageDataIteratorClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->finalize = finalize;
   gobject_class->set_property = set_property;
   gobject_class->get_property = get_property;

   g_object_class_install_property (gobject_class, PROP_IMAGE_DATA,
                                    g_param_spec_object ("image_data",
                                                         "ImageData",
                                                         "The GeglImageDataIterator's image data",
                                                          GEGL_TYPE_IMAGE_DATA,
                                                          G_PARAM_CONSTRUCT |
                                                          G_PARAM_READWRITE));

   g_object_class_install_property (gobject_class, PROP_AREA,
                                    g_param_spec_pointer ("area",
                                                          "Area",
                                                          "The (sub)rect area of the image data to process",
                                                          G_PARAM_CONSTRUCT |
                                                          G_PARAM_READWRITE));
}

static void 
init (GeglImageDataIterator * self, 
      GeglImageDataIteratorClass * klass)
{
  self->current = 0;
}

static void
finalize(GObject *gobject)
{
   GeglImageDataIterator *self = GEGL_IMAGE_DATA_ITERATOR (gobject);

   if(self->image_data)
     g_object_unref(self->image_data);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImageDataIterator *self = GEGL_IMAGE_DATA_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE_DATA:
      gegl_image_data_iterator_set_image_data (self, g_value_get_object(value));
      break;
    case PROP_AREA:
      gegl_image_data_iterator_set_rect (self, (GeglRect*)g_value_get_pointer (value));
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
  GeglImageDataIterator *self = GEGL_IMAGE_DATA_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE_DATA:
      g_value_set_object (value, gegl_image_data_iterator_get_image_data(self));
      break;
    case PROP_AREA:
      gegl_image_data_iterator_get_rect (self, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_image_data_iterator_get_image_data:
 * @self: a #GeglImageDataIterator
 *
 * Gets the #GeglImageData this iterator uses.
 *
 * Returns: a #GeglImageData this iterator uses. 
 **/
GeglImageData * 
gegl_image_data_iterator_get_image_data (GeglImageDataIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), NULL);
   
  return self->image_data;
}

void 
gegl_image_data_iterator_set_image_data (GeglImageDataIterator * self, 
                                         GeglImageData * image_data)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));

  if(image_data)
    g_object_ref(image_data);

  if(self->image_data)
    g_object_unref(self->image_data);
   
  self->image_data = image_data;
}

void 
gegl_image_data_iterator_get_rect (GeglImageDataIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));
  g_return_if_fail (rect != NULL);

  gegl_rect_copy(rect,&self->rect);
}

void 
gegl_image_data_iterator_set_rect (GeglImageDataIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));
  g_return_if_fail (rect!= NULL);

  gegl_rect_copy(&self->rect, rect);
}

/**
 * gegl_image_data_iterator_get_color_model:
 * @self: a #GeglImageDataIterator.
 *
 * Gets the color model of the image_data.
 *
 * Returns: the #GeglColorModel of the image_data. 
 **/
GeglColorModel * 
gegl_image_data_iterator_get_color_model (GeglImageDataIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), NULL);

  return gegl_image_data_get_color_model(self->image_data);
}

gint            
gegl_image_data_iterator_get_num_colors(GeglImageDataIterator * self)
{
  GeglColorModel *color_model;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), -1);

  color_model = gegl_image_data_get_color_model(self->image_data);
  return gegl_color_model_num_colors(color_model);
}

/**
 * gegl_image_data_iterator_first:
 * @self: a #GeglImageDataIterator.
 *
 * Makes the iterator point to the first scanline.
 *
 **/
void 
gegl_image_data_iterator_first (GeglImageDataIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));
  
  self->current = 0;
}

void 
gegl_image_data_iterator_set_scanline (GeglImageDataIterator * self, 
                                       gint y)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));
  g_return_if_fail (y >= self->rect.y && (y < (self->rect.y + self->rect.h)));
  self->current = y - self->rect.y;
}

/**
 * gegl_image_data_iterator_next:
 * @self: a #GeglImageDataIterator.
 *
 * Makes iterator point to the next scanline. 
 *
 **/
void 
gegl_image_data_iterator_next (GeglImageDataIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self));
   
  self->current++;
}

/**
 * gegl_image_data_iterator_is_done:
 * @self: a #GeglImageDataIterator.
 *
 * Checks if there are more scanlines. 
 *
 * Returns: TRUE if there are more scanlines.
 **/
gboolean 
gegl_image_data_iterator_is_done (GeglImageDataIterator * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), FALSE);
   
  if (self->current < self->rect.h) 
    return FALSE;
  else
    return TRUE;
}

/**
 * gegl_image_data_iterator_color_channels:
 * @self: a #GeglImageDataIterator.
 *
 * Gets an array of pointers to channel data. Should be freed.
 * If any channels are masked, they are passed back as NULL.
 *
 * Returns: An array of pointers to channel data.
 **/
gpointer *
gegl_image_data_iterator_color_channels(GeglImageDataIterator *self)
{
  gpointer * data_pointers; 
  gpointer * masked_data_pointers; 
  GeglColorModel *color_model;
  gint i;
  gint num_color_chans;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), NULL);

  tile = gegl_image_data_get_tile(self->image_data); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_data_get_color_model(self->image_data);
  num_color_chans = gegl_color_model_num_colors(color_model);

  masked_data_pointers = g_new(gpointer, num_color_chans); 

  for(i = 0; i < num_color_chans; i++)
    masked_data_pointers[i] = data_pointers[i];

  g_free(data_pointers);

  return masked_data_pointers;
}

/**
 * gegl_image_data_iterator_alpha_channel:
 * @self: a #GeglImageDataIterator.
 *
 * Gets a pointer to the alpha channel at current iterator location.
 * If the alpha channel is masked or non-existent, this is NULL.
 *
 * Returns: Pointer to alpha channel data.
 **/
gpointer
gegl_image_data_iterator_alpha_channel(GeglImageDataIterator *self)
{
  gint alpha_chan;
  gpointer *data_pointers;
  gpointer alpha_pointer;
  GeglColorModel *color_model;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_DATA_ITERATOR (self), NULL);

  tile = gegl_image_data_get_tile(self->image_data); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_data_get_color_model(self->image_data);
  alpha_chan = gegl_color_model_alpha_channel(color_model);

  if(alpha_chan >= 0)
    alpha_pointer = data_pointers[alpha_chan];
  else
    alpha_pointer = NULL;

  g_free(data_pointers);

  return alpha_pointer;
}
