#ifndef __GEGL_SUBTRACT_H__
#define __GEGL_SUBTRACT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-binary.h"

#define GEGL_TYPE_SUBTRACT               (gegl_subtract_get_type ())
#define GEGL_SUBTRACT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SUBTRACT, GeglSubtract))
#define GEGL_SUBTRACT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SUBTRACT, GeglSubtractClass))
#define GEGL_IS_SUBTRACT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SUBTRACT))
#define GEGL_IS_SUBTRACT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SUBTRACT))
#define GEGL_SUBTRACT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SUBTRACT, GeglSubtractClass))

#ifndef __TYPEDEF_GEGL_SUBTRACT__
#define __TYPEDEF_GEGL_SUBTRACT__
typedef struct _GeglSubtract GeglSubtract;
#endif
struct _GeglSubtract 
{
   GeglBinary binary;
   /*< private >*/
};

typedef struct _GeglSubtractClass GeglSubtractClass;
struct _GeglSubtractClass 
{
   GeglBinaryClass binary_class;
};

GType            gegl_subtract_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
