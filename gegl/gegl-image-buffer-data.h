#ifndef __GEGL_IMAGE_BUFFER_DATA_H__
#define __GEGL_IMAGE_BUFFER_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-data.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_IMAGE_BUFFER_DATA               (gegl_image_buffer_data_get_type ())
#define GEGL_IMAGE_BUFFER_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_BUFFER_DATA, GeglImageBufferData))
#define GEGL_IMAGE_BUFFER_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_BUFFER_DATA, GeglImageBufferDataClass))
#define GEGL_IS_IMAGE_BUFFER_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_BUFFER_DATA))
#define GEGL_IS_IMAGE_BUFFER_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_BUFFER_DATA))
#define GEGL_IMAGE_BUFFER_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_BUFFER_DATA, GeglImageBufferDataClass))

#ifndef __TYPEDEF_GEGL_IMAGE_BUFFER_DATA__
#define __TYPEDEF_GEGL_IMAGE_BUFFER_DATA__
typedef struct _GeglImageBufferData GeglImageBufferData;
#endif
struct _GeglImageBufferData 
{
    GeglData data;

    /*< private >*/
    GeglRect rect; 
    GeglColorModel *color_model;
};

typedef struct _GeglImageBufferDataClass GeglImageBufferDataClass;
struct _GeglImageBufferDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_image_buffer_data_get_type              (void);
void            gegl_image_buffer_data_get_rect              (GeglImageBufferData * self,
                                                             GeglRect * rect);
GeglColorModel* gegl_image_buffer_data_get_color_model       (GeglImageBufferData * self);
void            gegl_image_buffer_data_set_color_model       (GeglImageBufferData * self,
                                                             GeglColorModel *color_model);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
