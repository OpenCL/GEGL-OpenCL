#ifndef __GEGL_MULT_H__
#define __GEGL_MULT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_MULT               (gegl_mult_get_type ())
#define GEGL_MULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MULT, GeglMult))
#define GEGL_MULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MULT, GeglMultClass))
#define GEGL_IS_MULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MULT))
#define GEGL_IS_MULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MULT))
#define GEGL_MULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MULT, GeglMultClass))

#ifndef __TYPEDEF_GEGL_MULT__
#define __TYPEDEF_GEGL_MULT__
typedef struct _GeglMult GeglMult;
#endif
struct _GeglMult {
   GeglPointOp __parent__;
   /*< private >*/
};

typedef struct _GeglMultClass GeglMultClass;
struct _GeglMultClass {
   GeglPointOpClass __parent__;
};

GType            gegl_mult_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
