#ifndef __GEGL_STRING_DATA_H__
#define __GEGL_STRING_DATA_H__

#include "gegl-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_STRING_DATA               (gegl_string_data_get_type ())
#define GEGL_STRING_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_STRING_DATA, GeglStringData))
#define GEGL_STRING_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STRING_DATA, GeglStringDataClass))
#define GEGL_IS_STRING_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_STRING_DATA))
#define GEGL_IS_STRING_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STRING_DATA))
#define GEGL_STRING_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STRING_DATA, GeglStringDataClass))

typedef struct _GeglStringData GeglStringData;
struct _GeglStringData 
{
    GeglData data;

    /*< private >*/
};

typedef struct _GeglStringDataClass GeglStringDataClass;
struct _GeglStringDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_string_data_get_type              (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
