#ifndef __GEGL_COLOR_MODEL_GRAY_U8_H__
#define __GEGL_COLOR_MODEL_GRAY_U8_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-gray.h"

#define GEGL_TYPE_COLOR_MODEL_GRAY_U8               (gegl_color_model_gray_u8_get_type ())
#define GEGL_COLOR_MODEL_GRAY_U8(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_U8, GeglColorModelGrayU8))
#define GEGL_COLOR_MODEL_GRAY_U8_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_U8, GeglColorModelGrayU8Class))
#define GEGL_IS_COLOR_MODEL_GRAY_U8(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_U8))
#define GEGL_IS_COLOR_MODEL_GRAY_U8_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_U8))
#define GEGL_COLOR_MODEL_GRAY_U8_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_GRAY_U8, GeglColorModelGrayU8Class))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_GRAY_U8__
#define __TYPEDEF_GEGL_COLOR_MODEL_GRAY_U8__
typedef struct _GeglColorModelGrayU8 GeglColorModelGrayU8;
#endif
struct _GeglColorModelGrayU8 
{
   GeglColorModelGray color_model_gray;
};

typedef struct _GeglColorModelGrayU8Class GeglColorModelGrayU8Class;
struct _GeglColorModelGrayU8Class 
{
   GeglColorModelGrayClass color_model_gray_class;
};

GType                        gegl_color_model_gray_u8_get_type (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
