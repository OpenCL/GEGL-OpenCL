#ifndef __GEGL_LIGHT_IMPL_H__
#define __GEGL_LIGHT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_LIGHT_IMPL               (gegl_light_impl_get_type ())
#define GEGL_LIGHT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_LIGHT_IMPL, GeglLightImpl))
#define GEGL_LIGHT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_LIGHT_IMPL, GeglLightImplClass))
#define GEGL_IS_LIGHT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_LIGHT_IMPL))
#define GEGL_IS_LIGHT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_LIGHT_IMPL))
#define GEGL_LIGHT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_LIGHT_IMPL, GeglLightImplClass))

#ifndef __TYPEDEF_GEGL_LIGHT_IMPL__
#define __TYPEDEF_GEGL_LIGHT_IMPL__
typedef struct _GeglLightImpl GeglLightImpl;
#endif
struct _GeglLightImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglLightImplClass GeglLightImplClass;
struct _GeglLightImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_light_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
