#ifndef __GEGL_COLOR_MODEL_RGB_U8_H__
#define __GEGL_COLOR_MODEL_RGB_U8_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-rgb.h"

#define GEGL_TYPE_COLOR_MODEL_RGB_U8               (gegl_color_model_rgb_u8_get_type ())
#define GEGL_COLOR_MODEL_RGB_U8(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_RGB_U8, GeglColorModelRgbU8))
#define GEGL_COLOR_MODEL_RGB_U8_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_U8, GeglColorModelRgbU8Class))
#define GEGL_IS_COLOR_MODEL_RGB_U8(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_RGB_U8))
#define GEGL_IS_COLOR_MODEL_RGB_U8_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_RGB_U8))
#define GEGL_COLOR_MODEL_RGB_U8_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_RGB_U8, GeglColorModelRgbU8Class))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_RGB_U8__
#define __TYPEDEF_GEGL_COLOR_MODEL_RGB_U8__
typedef struct _GeglColorModelRgbU8 GeglColorModelRgbU8;
#endif
struct _GeglColorModelRgbU8 
{
   GeglColorModelRgb color_model_rgb;
};

typedef struct _GeglColorModelRgbU8Class GeglColorModelRgbU8Class;
struct _GeglColorModelRgbU8Class 
{
   GeglColorModelRgbClass color_model_rgb_class;
};

GType                       gegl_color_model_rgb_u8_get_type      (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
