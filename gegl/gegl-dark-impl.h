#ifndef __GEGL_DARK_IMPL_H__
#define __GEGL_DARK_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_DARK_IMPL               (gegl_dark_impl_get_type ())
#define GEGL_DARK_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DARK_IMPL, GeglDarkImpl))
#define GEGL_DARK_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DARK_IMPL, GeglDarkImplClass))
#define GEGL_IS_DARK_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DARK_IMPL))
#define GEGL_IS_DARK_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DARK_IMPL))
#define GEGL_DARK_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DARK_IMPL, GeglDarkImplClass))

#ifndef __TYPEDEF_GEGL_DARK_IMPL__
#define __TYPEDEF_GEGL_DARK_IMPL__
typedef struct _GeglDarkImpl GeglDarkImpl;
#endif
struct _GeglDarkImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglDarkImplClass GeglDarkImplClass;
struct _GeglDarkImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_dark_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
