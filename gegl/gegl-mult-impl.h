#ifndef __GEGL_MULT_IMPL_H__
#define __GEGL_MULT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_MULT_IMPL               (gegl_mult_impl_get_type ())
#define GEGL_MULT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MULT_IMPL, GeglMultImpl))
#define GEGL_MULT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MULT_IMPL, GeglMultImplClass))
#define GEGL_IS_MULT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MULT_IMPL))
#define GEGL_IS_MULT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MULT_IMPL))
#define GEGL_MULT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MULT_IMPL, GeglMultImplClass))

#ifndef __TYPEDEF_GEGL_MULT_IMPL__
#define __TYPEDEF_GEGL_MULT_IMPL__
typedef struct _GeglMultImpl GeglMultImpl;
#endif
struct _GeglMultImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglMultImplClass GeglMultImplClass;
struct _GeglMultImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_mult_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
