#ifndef __GEGL_COLOR_MODEL_GRAY_U16_H__
#define __GEGL_COLOR_MODEL_GRAY_U16_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-gray.h"

#define GEGL_TYPE_COLOR_MODEL_GRAY_U16               (gegl_color_model_gray_u16_get_type ())
#define GEGL_COLOR_MODEL_GRAY_U16(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_U16, GeglColorModelGrayU16))
#define GEGL_COLOR_MODEL_GRAY_U16_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_U16, GeglColorModelGrayU16Class))
#define GEGL_IS_COLOR_MODEL_GRAY_U16(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_U16))
#define GEGL_IS_COLOR_MODEL_GRAY_U16_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_U16))
#define GEGL_COLOR_MODEL_GRAY_U16_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_GRAY_U16, GeglColorModelGrayU16Class))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_GRAY_U16__
#define __TYPEDEF_GEGL_COLOR_MODEL_GRAY_U16__
typedef struct _GeglColorModelGrayU16 GeglColorModelGrayU16;
#endif
struct _GeglColorModelGrayU16 
{
   GeglColorModelGray color_model_gray;
};

typedef struct _GeglColorModelGrayU16Class GeglColorModelGrayU16Class;
struct _GeglColorModelGrayU16Class 
{
   GeglColorModelGrayClass color_model_gray_float;
};

GType                        gegl_color_model_gray_u16_get_type (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
