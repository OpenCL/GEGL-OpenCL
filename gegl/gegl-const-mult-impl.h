#ifndef __GEGL_CONST_MULT_IMPL_H__
#define __GEGL_CONST_MULT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#define GEGL_TYPE_CONST_MULT_IMPL               (gegl_const_mult_impl_get_type ())
#define GEGL_CONST_MULT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CONST_MULT_IMPL, GeglConstMultImpl))
#define GEGL_CONST_MULT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CONST_MULT_IMPL, GeglConstMultImplClass))
#define GEGL_IS_CONST_MULT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CONST_MULT_IMPL))
#define GEGL_IS_CONST_MULT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CONST_MULT_IMPL))
#define GEGL_CONST_MULT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CONST_MULT_IMPL, GeglConstMultImplClass))

#ifndef __TYPEDEF_GEGL_CONST_MULT_IMPL__
#define __TYPEDEF_GEGL_CONST_MULT_IMPL__
typedef struct _GeglConstMultImpl GeglConstMultImpl;
#endif
struct _GeglConstMultImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
   gfloat multiplier;
};

typedef struct _GeglConstMultImplClass GeglConstMultImplClass;
struct _GeglConstMultImplClass {
   GeglPointOpImplClass __parent__;
};

GType           gegl_const_mult_impl_get_type       (void);
gfloat          gegl_const_mult_impl_get_multiplier (GeglConstMultImpl * self);
void            gegl_const_mult_impl_set_multiplier (GeglConstMultImpl * self, 
                                                     gfloat multiplier);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
