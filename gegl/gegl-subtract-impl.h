#ifndef __GEGL_SUBTRACT_IMPL_H__
#define __GEGL_SUBTRACT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_SUBTRACT_IMPL               (gegl_subtract_impl_get_type ())
#define GEGL_SUBTRACT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SUBTRACT_IMPL, GeglSubtractImpl))
#define GEGL_SUBTRACT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SUBTRACT_IMPL, GeglSubtractImplClass))
#define GEGL_IS_SUBTRACT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SUBTRACT_IMPL))
#define GEGL_IS_SUBTRACT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SUBTRACT_IMPL))
#define GEGL_SUBTRACT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SUBTRACT_IMPL, GeglSubtractImplClass))

#ifndef __TYPEDEF_GEGL_SUBTRACT_IMPL__
#define __TYPEDEF_GEGL_SUBTRACT_IMPL__
typedef struct _GeglSubtractImpl GeglSubtractImpl;
#endif
struct _GeglSubtractImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglSubtractImplClass GeglSubtractImplClass;
struct _GeglSubtractImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_subtract_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
