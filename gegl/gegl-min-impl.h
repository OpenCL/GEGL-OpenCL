#ifndef __GEGL_MIN_IMPL_H__
#define __GEGL_MIN_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_MIN_IMPL               (gegl_min_impl_get_type ())
#define GEGL_MIN_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MIN_IMPL, GeglMinImpl))
#define GEGL_MIN_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MIN_IMPL, GeglMinImplClass))
#define GEGL_IS_MIN_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MIN_IMPL))
#define GEGL_IS_MIN_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MIN_IMPL))
#define GEGL_MIN_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MIN_IMPL, GeglMinImplClass))

#ifndef __TYPEDEF_GEGL_MIN_IMPL__
#define __TYPEDEF_GEGL_MIN_IMPL__
typedef struct _GeglMinImpl GeglMinImpl;
#endif
struct _GeglMinImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglMinImplClass GeglMinImplClass;
struct _GeglMinImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_min_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
