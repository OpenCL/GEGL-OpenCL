#ifndef __GEGL_COLOR_MODEL_GRAY_FLOAT_H__
#define __GEGL_COLOR_MODEL_GRAY_FLOAT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model-gray.h"

#define GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT               (gegl_color_model_gray_float_get_type ())
#define GEGL_COLOR_MODEL_GRAY_FLOAT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT, GeglColorModelGrayFloat))
#define GEGL_COLOR_MODEL_GRAY_FLOAT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT, GeglColorModelGrayFloatClass))
#define GEGL_IS_COLOR_MODEL_GRAY_FLOAT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT))
#define GEGL_IS_COLOR_MODEL_GRAY_FLOAT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT))
#define GEGL_COLOR_MODEL_GRAY_FLOAT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT, GeglColorModelGrayFloatClass))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_GRAY_FLOAT__
#define __TYPEDEF_GEGL_COLOR_MODEL_GRAY_FLOAT__
typedef struct _GeglColorModelGrayFloat GeglColorModelGrayFloat;
#endif
struct _GeglColorModelGrayFloat {
   GeglColorModelGray __parent__;
};

typedef struct _GeglColorModelGrayFloatClass GeglColorModelGrayFloatClass;
struct _GeglColorModelGrayFloatClass {
   GeglColorModelGrayClass __parent__;
};

GType                        gegl_color_model_gray_float_get_type (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
