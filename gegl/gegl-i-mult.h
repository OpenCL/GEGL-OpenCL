#ifndef __GEGL_I_MULT_H__
#define __GEGL_I_MULT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-binary.h"

#define GEGL_TYPE_I_MULT               (gegl_i_mult_get_type ())
#define GEGL_I_MULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_I_MULT, GeglIMult))
#define GEGL_I_MULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_I_MULT, GeglIMultClass))
#define GEGL_IS_I_MULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_I_MULT))
#define GEGL_IS_I_MULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_I_MULT))
#define GEGL_I_MULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_I_MULT, GeglIMultClass))

#ifndef __TYPEDEF_GEGL_I_MULT__
#define __TYPEDEF_GEGL_I_MULT__
typedef struct _GeglIMult GeglIMult;
#endif
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
