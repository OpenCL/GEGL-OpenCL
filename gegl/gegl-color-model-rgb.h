#ifndef __GEGL_COLOR_MODEL_RGB_H__
#define __GEGL_COLOR_MODEL_RGB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model.h"

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor GeglColor;
#endif

#define GEGL_TYPE_COLOR_MODEL_RGB               (gegl_color_model_rgb_get_type ())
#define GEGL_COLOR_MODEL_RGB(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_RGB, GeglColorModelRgb))
#define GEGL_COLOR_MODEL_RGB_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_RGB, GeglColorModelRgbClass))
#define GEGL_IS_COLOR_MODEL_RGB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_RGB))
#define GEGL_IS_COLOR_MODEL_RGB_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_RGB))
#define GEGL_COLOR_MODEL_RGB_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_RGB, GeglColorModelRgbClass))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_RGB__
#define __TYPEDEF_GEGL_COLOR_MODEL_RGB__
typedef struct _GeglColorModelRgb GeglColorModelRgb;
#endif
struct _GeglColorModelRgb {
   GeglColorModel __parent__;

   /*< private >*/
   gint red_index; 
   gint green_index;
   gint blue_index;
};

typedef struct _GeglColorModelRgbClass GeglColorModelRgbClass;
struct _GeglColorModelRgbClass {
   GeglColorModelClass __parent__;
};

GType   gegl_color_model_rgb_get_type          (void);
gint    gegl_color_model_rgb_get_red_index     (GeglColorModelRgb * self);
gint    gegl_color_model_rgb_get_green_index   (GeglColorModelRgb * self);
gint    gegl_color_model_rgb_get_blue_index    (GeglColorModelRgb * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
