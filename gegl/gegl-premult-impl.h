#ifndef __GEGL_PREMULT_IMPL_H__
#define __GEGL_PREMULT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_PREMULT_IMPL               (gegl_premult_impl_get_type ())
#define GEGL_PREMULT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PREMULT_IMPL, GeglPremultImpl))
#define GEGL_PREMULT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PREMULT_IMPL, GeglPremultImplClass))
#define GEGL_IS_PREMULT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PREMULT_IMPL))
#define GEGL_IS_PREMULT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PREMULT_IMPL))
#define GEGL_PREMULT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PREMULT_IMPL, GeglPremultImplClass))

#ifndef __TYPEDEF_GEGL_PREMULT_IMPL__
#define __TYPEDEF_GEGL_PREMULT_IMPL__
typedef struct _GeglPremultImpl GeglPremultImpl;
#endif
struct _GeglPremultImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
};

typedef struct _GeglPremultImplClass GeglPremultImplClass;
struct _GeglPremultImplClass {
   GeglPointOpImplClass __parent__;
};

GType           gegl_premult_impl_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
