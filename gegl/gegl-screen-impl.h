#ifndef __GEGL_SCREEN_IMPL_H__
#define __GEGL_SCREEN_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_SCREEN_IMPL               (gegl_screen_impl_get_type ())
#define GEGL_SCREEN_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SCREEN_IMPL, GeglScreenImpl))
#define GEGL_SCREEN_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SCREEN_IMPL, GeglScreenImplClass))
#define GEGL_IS_SCREEN_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SCREEN_IMPL))
#define GEGL_IS_SCREEN_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SCREEN_IMPL))
#define GEGL_SCREEN_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SCREEN_IMPL, GeglScreenImplClass))

#ifndef __TYPEDEF_GEGL_SCREEN_IMPL__
#define __TYPEDEF_GEGL_SCREEN_IMPL__
typedef struct _GeglScreenImpl GeglScreenImpl;
#endif
struct _GeglScreenImpl {
   GeglPointOpImpl __parent__;
   /*< private >*/
};

typedef struct _GeglScreenImplClass GeglScreenImplClass;
struct _GeglScreenImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_screen_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
