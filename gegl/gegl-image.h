#ifndef __GEGL_IMAGE_H__
#define __GEGL_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-filter.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE_BUFFER__
#define __TYPEDEF_GEGL_IMAGE_BUFFER__
typedef struct _GeglImageBuffer  GeglImageBuffer;
#endif

#define GEGL_TYPE_IMAGE               (gegl_image_get_type ())
#define GEGL_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE, GeglImage))
#define GEGL_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE, GeglImageClass))
#define GEGL_IS_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE))
#define GEGL_IS_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE))
#define GEGL_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE, GeglImageClass))

#ifndef __TYPEDEF_GEGL_IMAGE__
#define __TYPEDEF_GEGL_IMAGE__
typedef struct _GeglImage GeglImage;
#endif
struct _GeglImage 
{
   GeglFilter filter;

   /*< private >*/

   GeglColorModel * color_model;
   GeglImageBuffer * image_buffer;

   GeglColorModel * derived_color_model;
};

typedef struct _GeglImageClass GeglImageClass;
struct _GeglImageClass 
{
   GeglFilterClass filter_class;
};

GType           gegl_image_get_type             (void);
GeglColorModel* gegl_image_color_model          (GeglImage * self);
void            gegl_image_set_color_model      (GeglImage * self, 
                                                 GeglColorModel * cm);
void            gegl_image_set_derived_color_model(GeglImage * self, 
                                                   GeglColorModel * cm);

void            gegl_image_set_image_buffer (GeglImage * self, 
                                           GeglImageBuffer *image_buffer);
GeglImageBuffer * gegl_image_get_image_buffer (GeglImage * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
