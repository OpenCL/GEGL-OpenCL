#include "gegl-image-iterator.h"
#include "gegl-color-model.h"
#include "gegl-image.h"
#include "gegl-tile.h"

enum
{
  PROP_0, 
  PROP_IMAGE,
  PROP_AREA,
  PROP_LAST 
};

static void class_init (GeglImageIteratorClass * klass);
static void init (GeglImageIterator * self, GeglImageIteratorClass * klass);
static void finalize (GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_image_iterator_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageIteratorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageIterator),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglImageIterator", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglImageIteratorClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->finalize = finalize;
   gobject_class->set_property = set_property;
   gobject_class->get_property = get_property;

   g_object_class_install_property (gobject_class, PROP_IMAGE,
                                    g_param_spec_object ("image",
                                                         "Image",
                                                         "The GeglImageIterator's image data",
                                                          GEGL_TYPE_IMAGE,
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
init (GeglImageIterator * self, 
      GeglImageIteratorClass * klass)
{
  self->current = 0;
}

static void
finalize(GObject *gobject)
{
   GeglImageIterator *self = GEGL_IMAGE_ITERATOR (gobject);

   if(self->image)
     g_object_unref(self->image);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImageIterator *self = GEGL_IMAGE_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE:
      gegl_image_iterator_set_image (self, g_value_get_object(value));
      break;
    case PROP_AREA:
      gegl_image_iterator_set_rect (self, (GeglRect*)g_value_get_pointer (value));
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
  GeglImageIterator *self = GEGL_IMAGE_ITERATOR (gobject);

  switch (prop_id)
  {
    case PROP_IMAGE:
      g_value_set_object (value, gegl_image_iterator_get_image(self));
      break;
    case PROP_AREA:
      gegl_image_iterator_get_rect (self, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_image_iterator_get_image:
 * @self: a #GeglImageIterator
 *
 * Gets the #GeglImage this iterator uses.
 *
 * Returns: a #GeglImage this iterator uses. 
 **/
GeglImage * 
gegl_image_iterator_get_image (GeglImageIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), NULL);
   
  return self->image;
}

void 
gegl_image_iterator_set_image (GeglImageIterator * self, 
                                         GeglImage * image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));

  if(image)
    g_object_ref(image);

  if(self->image)
    g_object_unref(self->image);
   
  self->image = image;
}

void 
gegl_image_iterator_get_rect (GeglImageIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));
  g_return_if_fail (rect != NULL);

  gegl_rect_copy(rect,&self->rect);
}

void 
gegl_image_iterator_set_rect (GeglImageIterator * self, 
                                   GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));
  g_return_if_fail (rect!= NULL);

  gegl_rect_copy(&self->rect, rect);
}

/**
 * gegl_image_iterator_get_color_model:
 * @self: a #GeglImageIterator.
 *
 * Gets the color model of the image.
 *
 * Returns: the #GeglColorModel of the image. 
 **/
GeglColorModel * 
gegl_image_iterator_get_color_model (GeglImageIterator * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), NULL);

  return gegl_image_get_color_model(self->image);
}

gint            
gegl_image_iterator_get_num_colors(GeglImageIterator * self)
{
  GeglColorModel *color_model;
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), -1);

  color_model = gegl_image_get_color_model(self->image);
  return gegl_color_model_num_colors(color_model);
}

/**
 * gegl_image_iterator_first:
 * @self: a #GeglImageIterator.
 *
 * Makes the iterator point to the first scanline.
 *
 **/
void 
gegl_image_iterator_first (GeglImageIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));
  
  self->current = 0;
}

void 
gegl_image_iterator_set_scanline (GeglImageIterator * self, 
                                       gint y)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));
  g_return_if_fail (y >= self->rect.y && (y < (self->rect.y + self->rect.h)));
  self->current = y - self->rect.y;
}

/**
 * gegl_image_iterator_next:
 * @self: a #GeglImageIterator.
 *
 * Makes iterator point to the next scanline. 
 *
 **/
void 
gegl_image_iterator_next (GeglImageIterator * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_ITERATOR (self));
   
  self->current++;
}

/**
 * gegl_image_iterator_is_done:
 * @self: a #GeglImageIterator.
 *
 * Checks if there are more scanlines. 
 *
 * Returns: TRUE if there are more scanlines.
 **/
gboolean 
gegl_image_iterator_is_done (GeglImageIterator * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), FALSE);
   
  if (self->current < self->rect.h) 
    return FALSE;
  else
    return TRUE;
}

/**
 * gegl_image_iterator_color_channels:
 * @self: a #GeglImageIterator.
 *
 * Gets an array of pointers to channel data. Should be freed.
 * If any channels are masked, they are passed back as NULL.
 *
 * Returns: An array of pointers to channel data.
 **/
gpointer *
gegl_image_iterator_color_channels(GeglImageIterator *self)
{
  gpointer * data_pointers; 
  gpointer * masked_data_pointers; 
  GeglColorModel *color_model;
  gint i;
  gint num_color_chans;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), NULL);

  tile = gegl_image_get_tile(self->image); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_get_color_model(self->image);
  num_color_chans = gegl_color_model_num_colors(color_model);

  masked_data_pointers = g_new(gpointer, num_color_chans); 

  for(i = 0; i < num_color_chans; i++)
    masked_data_pointers[i] = data_pointers[i];

  g_free(data_pointers);

  return masked_data_pointers;
}

/**
 * gegl_image_iterator_alpha_channel:
 * @self: a #GeglImageIterator.
 *
 * Gets a pointer to the alpha channel at current iterator location.
 * If the alpha channel is masked or non-existent, this is NULL.
 *
 * Returns: Pointer to alpha channel data.
 **/
gpointer
gegl_image_iterator_alpha_channel(GeglImageIterator *self)
{
  gint alpha_chan;
  gpointer *data_pointers;
  gpointer alpha_pointer;
  GeglColorModel *color_model;
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_ITERATOR (self), NULL);

  tile = gegl_image_get_tile(self->image); 
  data_pointers = gegl_tile_data_pointers(tile,
                                            self->rect.x, 
                                            self->rect.y + self->current);  

  color_model = gegl_image_get_color_model(self->image);
  alpha_chan = gegl_color_model_alpha_channel(color_model);

  if(alpha_chan >= 0)
    alpha_pointer = data_pointers[alpha_chan];
  else
    alpha_pointer = NULL;

  g_free(data_pointers);

  return alpha_pointer;
}
