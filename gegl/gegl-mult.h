#ifndef __GEGL_MULT_H__
#define __GEGL_MULT_H__

#include "gegl-unary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MULT               (gegl_mult_get_type ())
#define GEGL_MULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MULT, GeglMult))
#define GEGL_MULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MULT, GeglMultClass))
#define GEGL_IS_MULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MULT))
#define GEGL_IS_MULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MULT))
#define GEGL_MULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MULT, GeglMultClass))

typedef struct _GeglMult GeglMult;
struct _GeglMult
{
   GeglUnary unary;

   /*< private >*/
};

typedef struct _GeglMultClass GeglMultClass;
struct _GeglMultClass
{
   GeglUnaryClass unary_class;
};

GType           gegl_mult_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
