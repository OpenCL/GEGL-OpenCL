#ifndef __GEGL_COMP_IMPL_H__
#define __GEGL_COMP_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_COMP_IMPL               (gegl_comp_impl_get_type ())
#define GEGL_COMP_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMP_IMPL, GeglCompImpl))
#define GEGL_COMP_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMP_IMPL, GeglCompImplClass))
#define GEGL_IS_COMP_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMP_IMPL))
#define GEGL_IS_COMP_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMP_IMPL))
#define GEGL_COMP_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMP_IMPL, GeglCompImplClass))

#ifndef __TYPEDEF_GEGL_COMP_IMPL__
#define __TYPEDEF_GEGL_COMP_IMPL__
typedef struct _GeglCompImpl GeglCompImpl;
#endif
struct _GeglCompImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
   GeglCompositeMode comp_mode; 
};

typedef struct _GeglCompImplClass GeglCompImplClass;
struct _GeglCompImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_comp_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
