#ifndef __GEGL_COLOR_H__
#define __GEGL_COLOR_H__

#include "gegl-no-input.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COLOR               (gegl_color_get_type ())
#define GEGL_COLOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR, GeglColor))
#define GEGL_COLOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR, GeglColorClass))
#define GEGL_IS_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR))
#define GEGL_IS_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR))
#define GEGL_COLOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR, GeglColorClass))

typedef struct _GeglColor GeglColor;
struct _GeglColor 
{
   GeglNoInput no_input;

   /*< private >*/
};

typedef struct _GeglColorClass GeglColorClass;
struct _GeglColorClass 
{
   GeglNoInputClass no_input_class;
};

GType           gegl_color_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
