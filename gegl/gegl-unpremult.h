#ifndef __GEGL_UNPREMULT_H__
#define __GEGL_UNPREMULT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_UNPREMULT               (gegl_unpremult_get_type ())
#define GEGL_UNPREMULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_UNPREMULT, GeglUnpremult))
#define GEGL_UNPREMULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_UNPREMULT, GeglUnpremultClass))
#define GEGL_IS_UNPREMULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_UNPREMULT))
#define GEGL_IS_UNPREMULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_UNPREMULT))
#define GEGL_UNPREMULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_UNPREMULT, GeglUnpremultClass))

#ifndef __TYPEDEF_GEGL_UNPREMULT__
#define __TYPEDEF_GEGL_UNPREMULT__
typedef struct _GeglUnpremult GeglUnpremult;
#endif
struct _GeglUnpremult {
   GeglPointOp __parent__;

   /*< private >*/
};

typedef struct _GeglUnpremultClass GeglUnpremultClass;
struct _GeglUnpremultClass {
   GeglPointOpClass __parent__;
};

GType           gegl_unpremult_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
