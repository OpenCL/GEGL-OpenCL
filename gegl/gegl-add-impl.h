#ifndef __GEGL_ADD_IMPL_H__
#define __GEGL_ADD_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_ADD_IMPL               (gegl_add_impl_get_type ())
#define GEGL_ADD_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_ADD_IMPL, GeglAddImpl))
#define GEGL_ADD_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_ADD_IMPL, GeglAddImplClass))
#define GEGL_IS_ADD_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_ADD_IMPL))
#define GEGL_IS_ADD_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_ADD_IMPL))
#define GEGL_ADD_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_ADD_IMPL, GeglAddImplClass))

#ifndef __TYPEDEF_GEGL_ADD_IMPL__
#define __TYPEDEF_GEGL_ADD_IMPL__
typedef struct _GeglAddImpl GeglAddImpl;
#endif
struct _GeglAddImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglAddImplClass GeglAddImplClass;
struct _GeglAddImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_add_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
