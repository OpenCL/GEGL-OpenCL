#ifndef __GEGL_CHECK_H__
#define __GEGL_CHECK_H__

#include "gegl-filter.h"
#include "gegl-image.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHECK               (gegl_check_get_type ())
#define GEGL_CHECK(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHECK, GeglCheck))
#define GEGL_CHECK_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHECK, GeglCheckClass))
#define GEGL_IS_CHECK(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHECK))
#define GEGL_IS_CHECK_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHECK))
#define GEGL_CHECK_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHECK, GeglCheckClass))

typedef struct _GeglCheck GeglCheck;
struct _GeglCheck 
{
    GeglFilter filter;

    /*< private >*/
    GeglImage *image;
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

void gegl_check_set_image (GeglCheck * self, GeglImage *image);
GeglImage * gegl_check_get_image (GeglCheck * self);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
