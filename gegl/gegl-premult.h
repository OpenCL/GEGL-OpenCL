#ifndef __GEGL_PREMULT_H__
#define __GEGL_PREMULT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_PREMULT               (gegl_premult_get_type ())
#define GEGL_PREMULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PREMULT, GeglPremult))
#define GEGL_PREMULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PREMULT, GeglPremultClass))
#define GEGL_IS_PREMULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PREMULT))
#define GEGL_IS_PREMULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PREMULT))
#define GEGL_PREMULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PREMULT, GeglPremultClass))

#ifndef __TYPEDEF_GEGL_PREMULT__
#define __TYPEDEF_GEGL_PREMULT__
typedef struct _GeglPremult GeglPremult;
#endif
struct _GeglPremult {
   GeglPointOp __parent__;

   /*< private >*/
};

typedef struct _GeglPremultClass GeglPremultClass;
struct _GeglPremultClass {
   GeglPointOpClass __parent__;
};

GType           gegl_premult_get_type       (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
