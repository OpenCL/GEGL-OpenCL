#ifndef __GEGL_DIFF_IMPL_H__
#define __GEGL_DIFF_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_DIFF_IMPL               (gegl_diff_impl_get_type ())
#define GEGL_DIFF_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DIFF_IMPL, GeglDiffImpl))
#define GEGL_DIFF_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DIFF_IMPL, GeglDiffImplClass))
#define GEGL_IS_DIFF_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DIFF_IMPL))
#define GEGL_IS_DIFF_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DIFF_IMPL))
#define GEGL_DIFF_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DIFF_IMPL, GeglDiffImplClass))

#ifndef __TYPEDEF_GEGL_DIFF_IMPL__
#define __TYPEDEF_GEGL_DIFF_IMPL__
typedef struct _GeglDiffImpl GeglDiffImpl;
#endif
struct _GeglDiffImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglDiffImplClass GeglDiffImplClass;
struct _GeglDiffImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_diff_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
