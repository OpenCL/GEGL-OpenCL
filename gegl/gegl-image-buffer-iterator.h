#ifndef __GEGL_IMAGE_BUFFER_ITERATOR_H__
#define __GEGL_IMAGE_BUFFER_ITERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_IMAGE_BUFFER__
#define __TYPEDEF_GEGL_IMAGE_BUFFER__
typedef struct _GeglImageBuffer GeglImageBuffer;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__ 
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_IMAGE_BUFFER_ITERATOR               (gegl_image_buffer_iterator_get_type ())
#define GEGL_IMAGE_BUFFER_ITERATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_BUFFER_ITERATOR, GeglImageBufferIterator))
#define GEGL_IMAGE_BUFFER_ITERATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_BUFFER_ITERATOR, GeglImageBufferIteratorClass))
#define GEGL_IS_IMAGE_BUFFER_ITERATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_BUFFER_ITERATOR))
#define GEGL_IS_IMAGE_BUFFER_ITERATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_BUFFER_ITERATOR))
#define GEGL_IMAGE_BUFFER_ITERATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_BUFFER_ITERATOR, GeglImageBufferIteratorClass))

#ifndef __TYPEDEF_GEGL_IMAGE_BUFFER_ITERATOR__
#define __TYPEDEF_GEGL_IMAGE_BUFFER_ITERATOR__
typedef struct _GeglImageBufferIterator GeglImageBufferIterator;
#endif
struct _GeglImageBufferIterator 
{
    GeglObject object;

    /*< private >*/
    GeglImageBuffer * image_buffer;
    GeglRect rect;
    gint current;
};

typedef struct _GeglImageBufferIteratorClass GeglImageBufferIteratorClass;
struct _GeglImageBufferIteratorClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_buffer_iterator_get_type     (void);
GeglImageBuffer * gegl_image_buffer_iterator_get_image_buffer (GeglImageBufferIterator * self);

void            gegl_image_buffer_iterator_get_rect     (GeglImageBufferIterator * self, 
                                                       GeglRect * rect);
GeglColorModel   *gegl_image_buffer_iterator_get_color_model(GeglImageBufferIterator * self);
gint            gegl_image_buffer_iterator_get_num_colors(GeglImageBufferIterator * self);

void            gegl_image_buffer_iterator_first        (GeglImageBufferIterator * self);
void            gegl_image_buffer_iterator_next         (GeglImageBufferIterator * self);
gboolean        gegl_image_buffer_iterator_is_done      (GeglImageBufferIterator * self);

gpointer *      gegl_image_buffer_iterator_color_channels(GeglImageBufferIterator *self);
gpointer        gegl_image_buffer_iterator_alpha_channel(GeglImageBufferIterator *self);

/* protected */
void            gegl_image_buffer_iterator_set_rect     (GeglImageBufferIterator * self, 
                                                       GeglRect * rect);
void            gegl_image_buffer_iterator_set_image_buffer (GeglImageBufferIterator * self, 
                                                         GeglImageBuffer * image_buffer);

void            gegl_image_buffer_iterator_set_scanline (GeglImageBufferIterator * self, 
                                                       gint y);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
