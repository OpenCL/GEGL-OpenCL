#ifndef __GEGL_MAX_H__
#define __GEGL_MAX_H__

#include "gegl-binary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MAX               (gegl_max_get_type ())
#define GEGL_MAX(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MAX, GeglMax))
#define GEGL_MAX_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MAX, GeglMaxClass))
#define GEGL_IS_MAX(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MAX))
#define GEGL_IS_MAX_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MAX))
#define GEGL_MAX_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MAX, GeglMaxClass))

typedef struct _GeglMax GeglMax;
struct _GeglMax 
{
   GeglBinary binary;
   /*< private >*/
};

typedef struct _GeglMaxClass GeglMaxClass;
struct _GeglMaxClass 
{
   GeglBinaryClass binary_class;
};

GType            gegl_max_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
