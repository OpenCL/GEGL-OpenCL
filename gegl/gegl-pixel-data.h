#ifndef __GEGL_PIXEL_DATA_H__
#define __GEGL_PIXEL_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-data.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_PIXEL_DATA               (gegl_pixel_data_get_type ())
#define GEGL_PIXEL_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PIXEL_DATA, GeglPixelData))
#define GEGL_PIXEL_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PIXEL_DATA, GeglPixelDataClass))
#define GEGL_IS_PIXEL_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PIXEL_DATA))
#define GEGL_IS_PIXEL_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PIXEL_DATA))
#define GEGL_PIXEL_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PIXEL_DATA, GeglPixelDataClass))

#ifndef __TYPEDEF_GEGL_PIXEL_DATA__
#define __TYPEDEF_GEGL_PIXEL_DATA__
typedef struct _GeglPixelData GeglPixelData;
#endif
struct _GeglPixelData 
{
    GeglData data;

    /*< private >*/
    GeglColorModel *color_model;
};

typedef struct _GeglPixelDataClass GeglPixelDataClass;
struct _GeglPixelDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_pixel_data_get_type              (void);
GeglColorModel* gegl_pixel_data_get_color_model       (GeglPixelData * self);
void            gegl_pixel_data_set_color_model       (GeglPixelData * self,
                                                       GeglColorModel *color_model);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
