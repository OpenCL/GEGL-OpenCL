#ifndef __GEGL_MIN_H__
#define __GEGL_MIN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-binary.h"

#define GEGL_TYPE_MIN               (gegl_min_get_type ())
#define GEGL_MIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MIN, GeglMin))
#define GEGL_MIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MIN, GeglMinClass))
#define GEGL_IS_MIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MIN))
#define GEGL_IS_MIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MIN))
#define GEGL_MIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MIN, GeglMinClass))

#ifndef __TYPEDEF_GEGL_MIN__
#define __TYPEDEF_GEGL_MIN__
typedef struct _GeglMin GeglMin;
#endif
struct _GeglMin 
{
   GeglBinary binary;
   /*< private >*/
};

typedef struct _GeglMinClass GeglMinClass;
struct _GeglMinClass 
{
   GeglBinaryClass binary_class;
};

GType            gegl_min_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
