#ifndef __GEGL_COPY_IMPL_H__
#define __GEGL_COPY_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"
#include "gegl-color-model.h"

#define GEGL_TYPE_COPY_IMPL               (gegl_copy_impl_get_type ())
#define GEGL_COPY_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COPY_IMPL, GeglCopyImpl))
#define GEGL_COPY_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COPY_IMPL, GeglCopyImplClass))
#define GEGL_IS_COPY_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COPY_IMPL))
#define GEGL_IS_COPY_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COPY_IMPL))
#define GEGL_COPY_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COPY_IMPL, GeglCopyImplClass))

#ifndef __TYPEDEF_GEGL_COPY_IMPL__
#define __TYPEDEF_GEGL_COPY_IMPL__
typedef struct _GeglCopyImpl GeglCopyImpl;
#endif
struct _GeglCopyImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
   gfloat * float_xyz_data[4];
   GeglConvertFunc convert_func;
};

typedef struct _GeglCopyImplClass GeglCopyImplClass;
struct _GeglCopyImplClass {
   GeglPointOpImplClass __parent__;
};

GType                gegl_copy_impl_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
