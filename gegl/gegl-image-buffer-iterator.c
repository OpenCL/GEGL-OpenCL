#include "gegl-image-buffer-iterator.h"
#include "gegl-color-model.h"
#include "gegl-image-buffer.h"
#include "gegl-tile.h"

enum
{
  PROP_0, 
  PROP_IMAGE_BUFFER,
  PROP_AREA,
  PROP_LAST 
};

static void class_init (GeglImageBufferIteratorClass * klass);
static void init (GeglImageBufferIterator * self, GeglImageBufferIteratorClass * klass);
static void finalize (GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_image_buffer_iterator_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageBufferIteratorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageBufferIterator),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglImageBufferIterator", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglImageBufferIteratorClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->finalize = finalize;
   gobject_class->set_property = set_property;
   gobject_class->get_property = get_property;

   g_object_class_install_property (gobject_class, PROP_IMAGE_BUFFER,
                                    g_param_spec_object ("image_buffer",
                                                         "ImageBuffer",
                                                         "The GeglImageBufferIterator's image data",
                                                          GEGL_TYPE_IMAGE_BUFFER,
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
init (GeglImageBufferIterator * self, 
      GeglImageBufferIteratorClass * klass)
{
  self->current = 0;
}

static void
finalize(GObject *gobject)
{
   GeglImageBufferIterator *self = GEGL_IMAGE_BUFFER_ITERATOR (gobject);

   if(self->image_buffer)
     g_object_unref(self->image_buffer);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImageBufferIterator *self = GEGL_IMAGE_BUFFER_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE_BUFFER:
      gegl_image_buffer_iterator_set_image_buffer (self, g_value_get_object(value));
      break;
    case PROP_AREA:
      gegl_image_buffer_iterator_set_rect (self, (GeglRect*)g_value_get_pointer (value));
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
  GeglImageBufferIterator *self = GEGL_IMAGE_BUFFER_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE_BUFFER:
      g_value_set_object (value, gegl_image_buffer_iterator_get_image_buffer(self));
      break;
    case PROP_AREA:
      gegl_image_buffer_iterator_get_rect (self, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_image_buffer_iterator_get_image_buffer:
 * @self: a #GeglImageBufferIterator
 *
 * Gets the #GeglImageBuffer this iterator uses.
 *
 * Returns: a #GeglImageBuffer this iterator uses. 
 **/
GeglImageBuffer * 
gegl_image_buffer_iterator_get_image_buffer (GeglImageBufferIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), NULL);
   
  return self->image_buffer;
}

void 
gegl_image_buffer_iterator_set_image_buffer (GeglImageBufferIterator * self, 
                                         GeglImageBuffer * image_buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));

  if(image_buffer)
    g_object_ref(image_buffer);

  if(self->image_buffer)
    g_object_unref(self->image_buffer);
   
  self->image_buffer = image_buffer;
}

void 
gegl_image_buffer_iterator_get_rect (GeglImageBufferIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));
  g_return_if_fail (rect != NULL);

  gegl_rect_copy(rect,&self->rect);
}

void 
gegl_image_buffer_iterator_set_rect (GeglImageBufferIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));
  g_return_if_fail (rect!= NULL);

  gegl_rect_copy(&self->rect, rect);
}

/**
 * gegl_image_buffer_iterator_get_color_model:
 * @self: a #GeglImageBufferIterator.
 *
 * Gets the color model of the image_buffer.
 *
 * Returns: the #GeglColorModel of the image_buffer. 
 **/
GeglColorModel * 
gegl_image_buffer_iterator_get_color_model (GeglImageBufferIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), NULL);

  return gegl_image_buffer_get_color_model(self->image_buffer);
}

gint            
gegl_image_buffer_iterator_get_num_colors(GeglImageBufferIterator * self)
{
  GeglColorModel *color_model;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), -1);

  color_model = gegl_image_buffer_get_color_model(self->image_buffer);
  return gegl_color_model_num_colors(color_model);
}

/**
 * gegl_image_buffer_iterator_first:
 * @self: a #GeglImageBufferIterator.
 *
 * Makes the iterator point to the first scanline.
 *
 **/
void 
gegl_image_buffer_iterator_first (GeglImageBufferIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));
  
  self->current = 0;
}

void 
gegl_image_buffer_iterator_set_scanline (GeglImageBufferIterator * self, 
                                       gint y)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));
  g_return_if_fail (y >= self->rect.y && (y < (self->rect.y + self->rect.h)));
  self->current = y - self->rect.y;
}

/**
 * gegl_image_buffer_iterator_next:
 * @self: a #GeglImageBufferIterator.
 *
 * Makes iterator point to the next scanline. 
 *
 **/
void 
gegl_image_buffer_iterator_next (GeglImageBufferIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self));
   
  self->current++;
}

/**
 * gegl_image_buffer_iterator_is_done:
 * @self: a #GeglImageBufferIterator.
 *
 * Checks if there are more scanlines. 
 *
 * Returns: TRUE if there are more scanlines.
 **/
gboolean 
gegl_image_buffer_iterator_is_done (GeglImageBufferIterator * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), FALSE);
   
  if (self->current < self->rect.h) 
    return FALSE;
  else
    return TRUE;
}

/**
 * gegl_image_buffer_iterator_color_channels:
 * @self: a #GeglImageBufferIterator.
 *
 * Gets an array of pointers to channel data. Should be freed.
 * If any channels are masked, they are passed back as NULL.
 *
 * Returns: An array of pointers to channel data.
 **/
gpointer *
gegl_image_buffer_iterator_color_channels(GeglImageBufferIterator *self)
{
  gpointer * data_pointers; 
  gpointer * masked_data_pointers; 
  GeglColorModel *color_model;
  gint i;
  gint num_color_chans;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), NULL);

  tile = gegl_image_buffer_get_tile(self->image_buffer); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_buffer_get_color_model(self->image_buffer);
  num_color_chans = gegl_color_model_num_colors(color_model);

  masked_data_pointers = g_new(gpointer, num_color_chans); 

  for(i = 0; i < num_color_chans; i++)
    masked_data_pointers[i] = data_pointers[i];

  g_free(data_pointers);

  return masked_data_pointers;
}

/**
 * gegl_image_buffer_iterator_alpha_channel:
 * @self: a #GeglImageBufferIterator.
 *
 * Gets a pointer to the alpha channel at current iterator location.
 * If the alpha channel is masked or non-existent, this is NULL.
 *
 * Returns: Pointer to alpha channel data.
 **/
gpointer
gegl_image_buffer_iterator_alpha_channel(GeglImageBufferIterator *self)
{
  gint alpha_chan;
  gpointer *data_pointers;
  gpointer alpha_pointer;
  GeglColorModel *color_model;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_ITERATOR (self), NULL);

  tile = gegl_image_buffer_get_tile(self->image_buffer); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_buffer_get_color_model(self->image_buffer);
  alpha_chan = gegl_color_model_alpha_channel(color_model);

  if(alpha_chan >= 0)
    alpha_pointer = data_pointers[alpha_chan];
  else
    alpha_pointer = NULL;

  g_free(data_pointers);

  return alpha_pointer;
}
