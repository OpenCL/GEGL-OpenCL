#ifndef __GEGL_IMAGE_IMPL_H__
#define __GEGL_IMAGE_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op-impl.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile  GeglTile;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_IMAGE_IMPL               (gegl_image_impl_get_type ())
#define GEGL_IMAGE_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_IMPL, GeglImageImpl))
#define GEGL_IMAGE_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_IMPL, GeglImageImplClass))
#define GEGL_IS_IMAGE_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_IMPL))
#define GEGL_IS_IMAGE_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_IMPL))
#define GEGL_IMAGE_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_IMPL, GeglImageImplClass))

#ifndef __TYPEDEF_GEGL_IMAGE_IMPL__
#define __TYPEDEF_GEGL_IMAGE_IMPL__
typedef struct _GeglImageImpl GeglImageImpl;
#endif
struct _GeglImageImpl {
   GeglOpImpl __parent__;

   /*< private >*/
   GeglColorModel * color_model;
   GeglTile * tile;
};

typedef struct _GeglImageImplClass GeglImageImplClass;
struct _GeglImageImplClass {
   GeglOpImplClass __parent__;
};

GType gegl_image_impl_get_type                     (void);

GeglColorModel * gegl_image_impl_color_model             (GeglImageImpl * self);
void             gegl_image_impl_set_color_model         (GeglImageImpl * self, 
                                                          GeglColorModel * cm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
