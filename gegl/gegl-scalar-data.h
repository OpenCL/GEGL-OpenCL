#ifndef __GEGL_SCALAR_DATA_H__
#define __GEGL_SCALAR_DATA_H__

#include "gegl-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_SCALAR_DATA               (gegl_scalar_data_get_type ())
#define GEGL_SCALAR_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SCALAR_DATA, GeglScalarData))
#define GEGL_SCALAR_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SCALAR_DATA, GeglScalarDataClass))
#define GEGL_IS_SCALAR_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SCALAR_DATA))
#define GEGL_IS_SCALAR_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SCALAR_DATA))
#define GEGL_SCALAR_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SCALAR_DATA, GeglScalarDataClass))

typedef struct _GeglScalarData GeglScalarData;
struct _GeglScalarData 
{
    GeglData data;

    /*< private >*/
};

typedef struct _GeglScalarDataClass GeglScalarDataClass;
struct _GeglScalarDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_scalar_data_get_type              (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
