#ifndef __GEGL_IMAGE_DATA_ITERATOR_H__
#define __GEGL_IMAGE_DATA_ITERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_IMAGE_DATA__
#define __TYPEDEF_GEGL_IMAGE_DATA__
typedef struct _GeglImageData GeglImageData;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__ 
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_IMAGE_DATA_ITERATOR               (gegl_image_data_iterator_get_type ())
#define GEGL_IMAGE_DATA_ITERATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_DATA_ITERATOR, GeglImageDataIterator))
#define GEGL_IMAGE_DATA_ITERATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_DATA_ITERATOR, GeglImageDataIteratorClass))
#define GEGL_IS_IMAGE_DATA_ITERATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_DATA_ITERATOR))
#define GEGL_IS_IMAGE_DATA_ITERATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_DATA_ITERATOR))
#define GEGL_IMAGE_DATA_ITERATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_DATA_ITERATOR, GeglImageDataIteratorClass))

#ifndef __TYPEDEF_GEGL_IMAGE_DATA_ITERATOR__
#define __TYPEDEF_GEGL_IMAGE_DATA_ITERATOR__
typedef struct _GeglImageDataIterator GeglImageDataIterator;
#endif
struct _GeglImageDataIterator 
{
    GeglObject object;

    /*< private >*/
    GeglImageData * image_data;
    GeglRect rect;
    gint current;
};

typedef struct _GeglImageDataIteratorClass GeglImageDataIteratorClass;
struct _GeglImageDataIteratorClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_data_iterator_get_type     (void);
GeglImageData * gegl_image_data_iterator_get_image_data (GeglImageDataIterator * self);

void            gegl_image_data_iterator_get_rect     (GeglImageDataIterator * self, 
                                                       GeglRect * rect);
GeglColorModel   *gegl_image_data_iterator_get_color_model(GeglImageDataIterator * self);
gint            gegl_image_data_iterator_get_num_colors(GeglImageDataIterator * self);

void            gegl_image_data_iterator_first        (GeglImageDataIterator * self);
void            gegl_image_data_iterator_next         (GeglImageDataIterator * self);
gboolean        gegl_image_data_iterator_is_done      (GeglImageDataIterator * self);

gpointer *      gegl_image_data_iterator_color_channels(GeglImageDataIterator *self);
gpointer        gegl_image_data_iterator_alpha_channel(GeglImageDataIterator *self);

/* protected */
void            gegl_image_data_iterator_set_rect     (GeglImageDataIterator * self, 
                                                       GeglRect * rect);
void            gegl_image_data_iterator_set_image_data (GeglImageDataIterator * self, 
                                                         GeglImageData * image_data);

void            gegl_image_data_iterator_set_scanline (GeglImageDataIterator * self, 
                                                       gint y);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
