#ifndef __GEGL_COLOR_MODEL_RGB_U16_H__
#define __GEGL_COLOR_MODEL_RGB_U16_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-rgb.h"

#define GEGL_TYPE_COLOR_MODEL_RGB_U16               (gegl_color_model_rgb_u16_get_type ())
#define GEGL_COLOR_MODEL_RGB_U16(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_RGB_U16, GeglColorModelRgbU16))
#define GEGL_COLOR_MODEL_RGB_U16_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_U16, GeglColorModelRgbU16Class))
#define GEGL_IS_COLOR_MODEL_RGB_U16(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_RGB_U16))
#define GEGL_IS_COLOR_MODEL_RGB_U16_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_U16))
#define GEGL_COLOR_MODEL_RGB_U16_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_RGB_U16, GeglColorModelRgbU16Class))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_RGB_U16__
#define __TYPEDEF_GEGL_COLOR_MODEL_RGB_U16__
typedef struct _GeglColorModelRgbU16 GeglColorModelRgbU16;
#endif
struct _GeglColorModelRgbU16 
{
   GeglColorModelRgb color_model_rgb;
};

typedef struct _GeglColorModelRgbU16Class GeglColorModelRgbU16Class;
struct _GeglColorModelRgbU16Class 
{
   GeglColorModelRgbClass color_model_rgb_class;
};

GType                       gegl_color_model_rgb_u16_get_type      (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
