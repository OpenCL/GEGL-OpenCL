#ifndef __GEGL_COLOR_SPACE_RGB_H__
#define __GEGL_COLOR_SPACE_RGB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-space.h"

#define GEGL_TYPE_COLOR_SPACE_RGB               (gegl_color_space_rgb_get_type ())
#define GEGL_COLOR_SPACE_RGB(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_SPACE_RGB, GeglColorSpaceRgb))
#define GEGL_COLOR_SPACE_RGB_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_SPACE_RGB, GeglColorSpaceRgbClass))
#define GEGL_IS_COLOR_SPACE_RGB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_SPACE_RGB))
#define GEGL_IS_COLOR_SPACE_RGB_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_SPACE_RGB))
#define GEGL_COLOR_SPACE_RGB_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_SPACE_RGB, GeglColorSpaceRgbClass))


typedef struct _GeglColorSpaceRgb GeglColorSpaceRgb;
struct _GeglColorSpaceRgb 
{
   GeglColorSpace color_space;
};


typedef struct _GeglColorSpaceRgbClass GeglColorSpaceRgbClass;
struct _GeglColorSpaceRgbClass 
{
   GeglColorSpaceClass color_space_class;
};


GType           gegl_color_space_rgb_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
