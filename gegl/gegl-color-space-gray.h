#ifndef __GEGL_COLOR_SPACE_GRAY_H__
#define __GEGL_COLOR_SPACE_GRAY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-space.h"

#define GEGL_TYPE_COLOR_SPACE_GRAY               (gegl_color_space_gray_get_type ())
#define GEGL_COLOR_SPACE_GRAY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_SPACE_GRAY, GeglColorSpaceGray))
#define GEGL_COLOR_SPACE_GRAY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_SPACE_GRAY, GeglColorSpaceGrayClass))
#define GEGL_IS_COLOR_SPACE_GRAY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_SPACE_GRAY))
#define GEGL_IS_COLOR_SPACE_GRAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_SPACE_GRAY))
#define GEGL_COLOR_SPACE_GRAY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_SPACE_GRAY, GeglColorSpaceGrayClass))


typedef struct _GeglColorSpaceGray GeglColorSpaceGray;
struct _GeglColorSpaceGray 
{
   GeglColorSpace color_space;
};


typedef struct _GeglColorSpaceGrayClass GeglColorSpaceGrayClass;
struct _GeglColorSpaceGrayClass 
{
   GeglColorSpaceClass color_space_class;
};


GType           gegl_color_space_gray_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
