#ifndef __GEGL_DATA_SPACE_U8_H__
#define __GEGL_DATA_SPACE_U8_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-data-space.h"

#define GEGL_TYPE_DATA_SPACE_U8               (gegl_data_space_u8_get_type ())
#define GEGL_DATA_SPACE_U8(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DATA_SPACE_U8, GeglDataSpaceU8))
#define GEGL_DATA_SPACE_U8_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DATA_SPACE_U8, GeglDataSpaceU8Class))
#define GEGL_IS_DATA_SPACE_U8(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DATA_SPACE_U8))
#define GEGL_IS_DATA_SPACE_U8_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DATA_SPACE_U8))
#define GEGL_DATA_SPACE_U8_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DATA_SPACE_U8, GeglDataSpaceU8Class))


typedef struct _GeglDataSpaceU8 GeglDataSpaceU8;
struct _GeglDataSpaceU8 
{
   GeglDataSpace data_space;
};


typedef struct _GeglDataSpaceU8Class GeglDataSpaceU8Class;
struct _GeglDataSpaceU8Class 
{
  GeglDataSpaceClass data_space_class;
};


GType           gegl_data_space_u8_get_type       (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
