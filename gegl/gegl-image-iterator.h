#ifndef __GEGL_IMAGE_ITERATOR_H__
#define __GEGL_IMAGE_ITERATOR_H__

#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-image.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_IMAGE_ITERATOR               (gegl_image_iterator_get_type ())
#define GEGL_IMAGE_ITERATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_ITERATOR, GeglImageIterator))
#define GEGL_IMAGE_ITERATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_ITERATOR, GeglImageIteratorClass))
#define GEGL_IS_IMAGE_ITERATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_ITERATOR))
#define GEGL_IS_IMAGE_ITERATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_ITERATOR))
#define GEGL_IMAGE_ITERATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_ITERATOR, GeglImageIteratorClass))

typedef struct _GeglImageIterator GeglImageIterator;
struct _GeglImageIterator 
{
    GeglObject object;

    /*< private >*/
    GeglImage * image;
    GeglRect rect;
    gint current;
};

typedef struct _GeglImageIteratorClass GeglImageIteratorClass;
struct _GeglImageIteratorClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_iterator_get_type     (void);
GeglImage * gegl_image_iterator_get_image (GeglImageIterator * self);

void            gegl_image_iterator_get_rect     (GeglImageIterator * self, 
                                                       GeglRect * rect);
GeglColorModel   *gegl_image_iterator_get_color_model(GeglImageIterator * self);
gint            gegl_image_iterator_get_num_colors(GeglImageIterator * self);

void            gegl_image_iterator_first        (GeglImageIterator * self);
void            gegl_image_iterator_next         (GeglImageIterator * self);
gboolean        gegl_image_iterator_is_done      (GeglImageIterator * self);

gpointer *      gegl_image_iterator_color_channels(GeglImageIterator *self);
gpointer        gegl_image_iterator_alpha_channel(GeglImageIterator *self);

/* protected */
void            gegl_image_iterator_set_rect     (GeglImageIterator * self, 
                                                       GeglRect * rect);
void            gegl_image_iterator_set_image (GeglImageIterator * self, 
                                                         GeglImage * image);

void            gegl_image_iterator_set_scanline (GeglImageIterator * self, 
                                                       gint y);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
