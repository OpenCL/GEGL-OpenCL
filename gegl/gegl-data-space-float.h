#ifndef __GEGL_DATA_SPACE_FLOAT_H__
#define __GEGL_DATA_SPACE_FLOAT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-data-space.h"

#define GEGL_TYPE_DATA_SPACE_FLOAT               (gegl_data_space_float_get_type ())
#define GEGL_DATA_SPACE_FLOAT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DATA_SPACE_FLOAT, GeglDataSpaceFloat))
#define GEGL_DATA_SPACE_FLOAT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DATA_SPACE_FLOAT, GeglDataSpaceFloatClass))
#define GEGL_IS_DATA_SPACE_FLOAT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DATA_SPACE_FLOAT))
#define GEGL_IS_DATA_SPACE_FLOAT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DATA_SPACE_FLOAT))
#define GEGL_DATA_SPACE_FLOAT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DATA_SPACE_FLOAT, GeglDataSpaceFloatClass))


typedef struct _GeglDataSpaceFloat GeglDataSpaceFloat;
struct _GeglDataSpaceFloat 
{
   GeglDataSpace data_space;
};


typedef struct _GeglDataSpaceFloatClass GeglDataSpaceFloatClass;
struct _GeglDataSpaceFloatClass 
{
  GeglDataSpaceClass data_space_class;
};


GType           gegl_data_space_float_get_type       (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
