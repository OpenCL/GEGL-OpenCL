#ifndef __GEGL_COLOR_DATA_H__
#define __GEGL_COLOR_DATA_H__


#include "gegl-data.h"
#include "gegl-color-space.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COLOR_DATA               (gegl_color_data_get_type ())
#define GEGL_COLOR_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_DATA, GeglColorData))
#define GEGL_COLOR_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_DATA, GeglColorDataClass))
#define GEGL_IS_COLOR_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_DATA))
#define GEGL_IS_COLOR_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_DATA))
#define GEGL_COLOR_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_DATA, GeglColorDataClass))

typedef struct _GeglColorData GeglColorData;
struct _GeglColorData 
{
    GeglData data;

    /*< private >*/
    GeglColorSpace *color_space;
};

typedef struct _GeglColorDataClass GeglColorDataClass;
struct _GeglColorDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_color_data_get_type              (void);

GeglColorSpace* gegl_color_data_get_color_space       (GeglColorData * self);
void            gegl_color_data_set_color_space       (GeglColorData * self,
                                                       GeglColorSpace *color_space);
gfloat         *gegl_color_data_get_components        (GeglColorData *self,
                                                       gint *num_components);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
