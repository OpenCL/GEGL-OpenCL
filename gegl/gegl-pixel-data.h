#ifndef __GEGL_PIXEL_DATA_H__
#define __GEGL_PIXEL_DATA_H__

#include "gegl-color-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_PIXEL_DATA               (gegl_pixel_data_get_type ())
#define GEGL_PIXEL_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PIXEL_DATA, GeglPixelData))
#define GEGL_PIXEL_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PIXEL_DATA, GeglPixelDataClass))
#define GEGL_IS_PIXEL_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PIXEL_DATA))
#define GEGL_IS_PIXEL_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PIXEL_DATA))
#define GEGL_PIXEL_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PIXEL_DATA, GeglPixelDataClass))

typedef struct _GeglPixelData GeglPixelData;
struct _GeglPixelData 
{
    GeglColorData data;

    /*< private >*/
};

typedef struct _GeglPixelDataClass GeglPixelDataClass;
struct _GeglPixelDataClass 
{
    GeglColorDataClass data_class;
};

GType           gegl_pixel_data_get_type              (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
