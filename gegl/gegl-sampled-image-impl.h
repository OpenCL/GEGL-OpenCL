#ifndef __GEGL_SAMPLED_IMAGE_IMPL_H__
#define __GEGL_SAMPLED_IMAGE_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image-impl.h"

#define GEGL_TYPE_SAMPLED_IMAGE_IMPL               (gegl_sampled_image_impl_get_type ())
#define GEGL_SAMPLED_IMAGE_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLED_IMAGE_IMPL, GeglSampledImageImpl))
#define GEGL_SAMPLED_IMAGE_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLED_IMAGE_IMPL, GeglSampledImageImplClass))
#define GEGL_IS_SAMPLED_IMAGE_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLED_IMAGE_IMPL))
#define GEGL_IS_SAMPLED_IMAGE_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLED_IMAGE_IMPL))
#define GEGL_SAMPLED_IMAGE_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLED_IMAGE_IMPL, GeglSampledImageImplClass))

#ifndef __TYPEDEF_GEGL_SAMPLED_IMAGE_IMPL__
#define __TYPEDEF_GEGL_SAMPLED_IMAGE_IMPL__
typedef struct _GeglSampledImageImpl GeglSampledImageImpl;
#endif
struct _GeglSampledImageImpl {
   GeglImageImpl __parent__;

   /*< private >*/
   gint width;  
   gint height;
};

typedef struct _GeglSampledImageImplClass GeglSampledImageImplClass;
struct _GeglSampledImageImplClass {
   GeglImageImplClass __parent__;
};

GType                gegl_sampled_image_impl_get_type    (void);

gint                 gegl_sampled_image_impl_get_width   (GeglSampledImageImpl * self);
gint                 gegl_sampled_image_impl_get_height  (GeglSampledImageImpl * self);
void                 gegl_sampled_image_impl_set_width   (GeglSampledImageImpl * self, 
                                                           gint width);
void                 gegl_sampled_image_impl_set_height  (GeglSampledImageImpl * self, 
                                                           gint height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
