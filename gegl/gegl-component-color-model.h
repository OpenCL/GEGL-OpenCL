#ifndef __GEGL_COMPONENT_COLOR_MODEL_H__
#define __GEGL_COMPONENT_COLOR_MODEL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-color-model.h"

#ifndef __TYPEDEF_GEGL_COLOR_SPACE__
#define __TYPEDEF_GEGL_COLOR_SPACE__
typedef struct _GeglColorSpace  GeglColorSpace;
#endif

#ifndef __TYPEDEF_GEGL_DATA_SPACE__
#define __TYPEDEF_GEGL_DATA_SPACE__
typedef struct _GeglDataSpace  GeglDataSpace;
#endif

#define GEGL_TYPE_COMPONENT_COLOR_MODEL               (gegl_component_color_model_get_type ())
#define GEGL_COMPONENT_COLOR_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModel))
#define GEGL_COMPONENT_COLOR_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModelClass))
#define GEGL_IS_COMPONENT_COLOR_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMPONENT_COLOR_MODEL))
#define GEGL_IS_COMPONENT_COLOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMPONENT_COLOR_MODEL))
#define GEGL_COMPONENT_COLOR_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModelClass))


#ifndef __TYPEDEF_GEGL_COMPONENT_COLOR_MODEL__
#define __TYPEDEF_GEGL_COMPONENT_COLOR_MODEL__
typedef struct _GeglComponentColorModel  GeglComponentColorModel;
#endif
struct _GeglComponentColorModel 
{
   GeglColorModel color_model;

   /*< private >*/
};


typedef struct _GeglComponentColorModelClass GeglComponentColorModelClass;
struct _GeglComponentColorModelClass 
{
   GeglColorModelClass color_model_class;
};


GType           gegl_component_color_model_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
