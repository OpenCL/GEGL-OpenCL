#ifndef __GEGL_COMP_PREMULT_IMPL_H__
#define __GEGL_COMP_PREMULT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_COMP_PREMULT_IMPL               (gegl_comp_premult_impl_get_type ())
#define GEGL_COMP_PREMULT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMP_PREMULT_IMPL, GeglCompPremultImpl))
#define GEGL_COMP_PREMULT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMP_PREMULT_IMPL, GeglCompPremultImplClass))
#define GEGL_IS_COMP_PREMULT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMP_PREMULT_IMPL))
#define GEGL_IS_COMP_PREMULT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMP_PREMULT_IMPL))
#define GEGL_COMP_PREMULT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMP_PREMULT_IMPL, GeglCompPremultImplClass))

#ifndef __TYPEDEF_GEGL_COMP_PREMULT_IMPL__
#define __TYPEDEF_GEGL_COMP_PREMULT_IMPL__
typedef struct _GeglCompPremultImpl GeglCompPremultImpl;
#endif
struct _GeglCompPremultImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
   GeglCompositeMode comp_mode; 
};

typedef struct _GeglCompPremultImplClass GeglCompPremultImplClass;
struct _GeglCompPremultImplClass {
   GeglPointOpImplClass __parent__;
};

GType            gegl_comp_premult_impl_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
