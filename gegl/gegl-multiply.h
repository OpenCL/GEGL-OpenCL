#ifndef __GEGL_MULTIPLY_H__
#define __GEGL_MULTIPLY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-blend.h"

#define GEGL_TYPE_MULTIPLY               (gegl_multiply_get_type ())
#define GEGL_MULTIPLY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MULTIPLY, GeglMultiply))
#define GEGL_MULTIPLY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MULTIPLY, GeglMultiplyClass))
#define GEGL_IS_MULTIPLY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MULTIPLY))
#define GEGL_IS_MULTIPLY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MULTIPLY))
#define GEGL_MULTIPLY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MULTIPLY, GeglMultiplyClass))

#ifndef __TYPEDEF_GEGL_MULTIPLY__
#define __TYPEDEF_GEGL_MULTIPLY__
typedef struct _GeglMultiply GeglMultiply;
#endif
struct _GeglMultiply 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglMultiplyClass GeglMultiplyClass;
struct _GeglMultiplyClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_multiply_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
