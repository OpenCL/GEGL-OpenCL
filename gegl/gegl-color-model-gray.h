#ifndef __GEGL_COLOR_MODEL_GRAY_H__
#define __GEGL_COLOR_MODEL_GRAY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model.h"

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor GeglColor;
#endif

#define GEGL_TYPE_COLOR_MODEL_GRAY               (gegl_color_model_gray_get_type ())
#define GEGL_COLOR_MODEL_GRAY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL_GRAY, GeglColorModelGray))
#define GEGL_COLOR_MODEL_GRAY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY, GeglColorModelGrayClass))
#define GEGL_IS_COLOR_MODEL_GRAY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL_GRAY))
#define GEGL_IS_COLOR_MODEL_GRAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL_GRAY))
#define GEGL_COLOR_MODEL_GRAY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL_GRAY, GeglColorModelGrayClass))

#ifndef __TYPEDEF_GEGL_COLOR_MODEL_GRAY__
#define __TYPEDEF_GEGL_COLOR_MODEL_GRAY__
typedef struct _GeglColorModelGray GeglColorModelGray;
#endif
struct _GeglColorModelGray 
{
   GeglColorModel color_model;
   /*< private >*/
   gint gray_index;
};

typedef struct _GeglColorModelGrayClass GeglColorModelGrayClass;
struct _GeglColorModelGrayClass 
{
   GeglColorModelClass color_model_class;
};

GType   gegl_color_model_gray_get_type         (void);
gint    gegl_color_model_gray_get_gray_index   (GeglColorModelGray * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
