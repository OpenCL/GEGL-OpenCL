#ifndef __GEGL_FILL_IMPL_H__
#define __GEGL_FILL_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op-impl.h"

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor GeglColor;
#endif

#define GEGL_TYPE_FILL_IMPL               (gegl_fill_impl_get_type ())
#define GEGL_FILL_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILL_IMPL, GeglFillImpl))
#define GEGL_FILL_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILL_IMPL, GeglFillImplClass))
#define GEGL_IS_FILL_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILL_IMPL))
#define GEGL_IS_FILL_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILL_IMPL))
#define GEGL_FILL_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILL_IMPL, GeglFillImplClass))

#ifndef __TYPEDEF_GEGL_FILL_IMPL__
#define __TYPEDEF_GEGL_FILL_IMPL__
typedef struct _GeglFillImpl GeglFillImpl;
#endif
struct _GeglFillImpl {
   GeglPointOpImpl __parent__;

   /*< private >*/
   GeglColor * fill_color;
};

typedef struct _GeglFillImplClass GeglFillImplClass;
struct _GeglFillImplClass {
   GeglPointOpImplClass __parent__;
};

GType           gegl_fill_impl_get_type              (void);
GeglColor *     gegl_fill_impl_get_fill_color        (GeglFillImpl * self);
void            gegl_fill_impl_set_fill_color        (GeglFillImpl * self, 
                                                        GeglColor *fill_color);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
