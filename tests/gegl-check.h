#ifndef __GEGL_CHECK_H__
#define __GEGL_CHECK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-filter.h"

#ifndef __TYPEDEF_GEGL_IMAGE_DATA__
#define __TYPEDEF_GEGL_IMAGE_DATA__
typedef struct _GeglImageData GeglImageData;
#endif

#define GEGL_TYPE_CHECK               (gegl_check_get_type ())
#define GEGL_CHECK(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHECK, GeglCheck))
#define GEGL_CHECK_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHECK, GeglCheckClass))
#define GEGL_IS_CHECK(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHECK))
#define GEGL_IS_CHECK_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHECK))
#define GEGL_CHECK_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHECK, GeglCheckClass))

#ifndef __TYPEDEF_GEGL_CHECK__
#define __TYPEDEF_GEGL_CHECK__
typedef struct _GeglCheck GeglCheck;
#endif
struct _GeglCheck 
{
    GeglFilter filter;

    /*< private >*/
    GeglImageData *image_data;
    GValue *pixel;
    gboolean success;
    gint x;
    gint y;
};

typedef struct _GeglCheckClass GeglCheckClass;
struct _GeglCheckClass 
{
    GeglFilterClass filter_class;
};

GType           gegl_check_get_type           (void);

gboolean gegl_check_get_success(GeglCheck *self);
void gegl_check_get_pixel (GeglCheck * self, GValue *pixel);
void gegl_check_set_pixel (GeglCheck * self, GValue *pixel);

void gegl_check_set_image_data (GeglCheck * self, GeglImageData *image_data);
GeglImageData * gegl_check_get_image_data (GeglCheck * self);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
