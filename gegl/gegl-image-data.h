#ifndef __GEGL_IMAGE_DATA_H__
#define __GEGL_IMAGE_DATA_H__

#include "gegl-color-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_IMAGE_DATA               (gegl_image_data_get_type ())
#define GEGL_IMAGE_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_DATA, GeglImageData))
#define GEGL_IMAGE_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_DATA, GeglImageDataClass))
#define GEGL_IS_IMAGE_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_DATA))
#define GEGL_IS_IMAGE_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_DATA))
#define GEGL_IMAGE_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_DATA, GeglImageDataClass))

typedef struct _GeglImageData GeglImageData;
struct _GeglImageData 
{
    GeglColorData data;

    /*< private >*/
    GeglRect rect; 
};

typedef struct _GeglImageDataClass GeglImageDataClass;
struct _GeglImageDataClass 
{
    GeglColorDataClass data_class;
};

GType           gegl_image_data_get_type              (void);
void            gegl_image_data_get_rect              (GeglImageData * self,
                                                       GeglRect * rect);
void            gegl_image_data_set_rect              (GeglImageData * self,
                                                       GeglRect * rect);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
