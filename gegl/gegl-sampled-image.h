#ifndef __GEGL_SAMPLED_IMAGE_H__
#define __GEGL_SAMPLED_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image.h"

#define GEGL_TYPE_SAMPLED_IMAGE               (gegl_sampled_image_get_type ())
#define GEGL_SAMPLED_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLED_IMAGE, GeglSampledImage))
#define GEGL_SAMPLED_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLED_IMAGE, GeglSampledImageClass))
#define GEGL_IS_SAMPLED_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLED_IMAGE))
#define GEGL_IS_SAMPLED_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLED_IMAGE))
#define GEGL_SAMPLED_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLED_IMAGE, GeglSampledImageClass))

#ifndef __TYPEDEF_GEGL_SAMPLED_IMAGE__
#define __TYPEDEF_GEGL_SAMPLED_IMAGE__
typedef struct _GeglSampledImage GeglSampledImage;
#endif
struct _GeglSampledImage {
   GeglImage __parent__;

   /*< private >*/
   gint width;  
   gint height;
};

typedef struct _GeglSampledImageClass GeglSampledImageClass;
struct _GeglSampledImageClass {
   GeglImageClass __parent__;
};

GType                gegl_sampled_image_get_type    (void);
gint                 gegl_sampled_image_get_width   (GeglSampledImage * self);
gint                 gegl_sampled_image_get_height  (GeglSampledImage * self);

void                 gegl_sampled_image_set_width   (GeglSampledImage * self, gint width);
void                 gegl_sampled_image_set_height  (GeglSampledImage * self, gint height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
