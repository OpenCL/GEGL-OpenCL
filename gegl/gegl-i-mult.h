#ifndef __GEGL_I_MULT_H__
#define __GEGL_I_MULT_H__

#include "gegl-binary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_I_MULT               (gegl_i_mult_get_type ())
#define GEGL_I_MULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_I_MULT, GeglIMult))
#define GEGL_I_MULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_I_MULT, GeglIMultClass))
#define GEGL_IS_I_MULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_I_MULT))
#define GEGL_IS_I_MULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_I_MULT))
#define GEGL_I_MULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_I_MULT, GeglIMultClass))

typedef struct _GeglIMult GeglIMult;
struct _GeglIMult 
{
   GeglBinary binary;
   /*< private >*/
};

typedef struct _GeglIMultClass GeglIMultClass;
struct _GeglIMultClass 
{
   GeglBinaryClass binary_class;
};

GType            gegl_i_mult_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
