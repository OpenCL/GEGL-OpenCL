#ifndef __GEGL_COMPONENT_COLOR_MODEL_H__
#define __GEGL_COMPONENT_COLOR_MODEL_H__

#include "gegl-color-model.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COMPONENT_COLOR_MODEL               (gegl_component_color_model_get_type ())
#define GEGL_COMPONENT_COLOR_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModel))
#define GEGL_COMPONENT_COLOR_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModelClass))
#define GEGL_IS_COMPONENT_COLOR_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMPONENT_COLOR_MODEL))
#define GEGL_IS_COMPONENT_COLOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMPONENT_COLOR_MODEL))
#define GEGL_COMPONENT_COLOR_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMPONENT_COLOR_MODEL, GeglComponentColorModelClass))


typedef struct _GeglComponentColorModel  GeglComponentColorModel;
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
