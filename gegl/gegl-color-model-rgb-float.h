#ifndef __GEGL_COLOR_MODEL_RGB_FLOAT_H__
#define __GEGL_COLOR_MODEL_RGB_FLOAT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-rgb.h"

#define GEGL_TYPE_COLOR_MODEL_RGB_FLOAT               (gegl_color_model_rgb_float_get_type ())
#define GEGL_COLOR_MODEL_RGB_FLOAT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, GeglColorModelRgbFloat))
#define GEGL_COLOR_MODEL_RGB_FLOAT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, GeglColorModelRgbFloatClass))
#define GEGL_IS_COLOR_MODEL_RGB_FLOAT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_RGB_FLOAT))
#define GEGL_IS_COLOR_MODEL_RGB_FLOAT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_FLOAT))
#define GEGL_COLOR_MODEL_RGB_FLOAT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, GeglColorModelRgbFloatClass))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_RGB_FLOAT__
#define __TYPEDEF_GEGL_COLOR_MODEL_RGB_FLOAT__
typedef struct _GeglColorModelRgbFloat GeglColorModelRgbFloat;
#endif
struct _GeglColorModelRgbFloat {
   GeglColorModelRgb __parent__;
};

typedef struct _GeglColorModelRgbFloatClass GeglColorModelRgbFloatClass;
struct _GeglColorModelRgbFloatClass {
   GeglColorModelRgbClass __parent__;
};

GType                       gegl_color_model_rgb_float_get_type      (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
