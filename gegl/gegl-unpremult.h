#ifndef __GEGL_UNPREMULT_H__
#define __GEGL_UNPREMULT_H__

#include "gegl-unary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_UNPREMULT               (gegl_unpremult_get_type ())
#define GEGL_UNPREMULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_UNPREMULT, GeglUnpremult))
#define GEGL_UNPREMULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_UNPREMULT, GeglUnpremultClass))
#define GEGL_IS_UNPREMULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_UNPREMULT))
#define GEGL_IS_UNPREMULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_UNPREMULT))
#define GEGL_UNPREMULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_UNPREMULT, GeglUnpremultClass))

typedef struct _GeglUnpremult GeglUnpremult;
struct _GeglUnpremult 
{
   GeglUnary unary;
   /*< private >*/
};

typedef struct _GeglUnpremultClass GeglUnpremultClass;
struct _GeglUnpremultClass 
{
   GeglUnaryClass unary_class;
};

GType            gegl_unpremult_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
