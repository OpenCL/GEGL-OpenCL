#ifndef __GEGL_OUTSIDE_H__
#define __GEGL_OUTSIDE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-comp.h"

#define GEGL_TYPE_OUTSIDE               (gegl_outside_get_type ())
#define GEGL_OUTSIDE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OUTSIDE, GeglOutside))
#define GEGL_OUTSIDE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OUTSIDE, GeglOutsideClass))
#define GEGL_IS_OUTSIDE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OUTSIDE))
#define GEGL_IS_OUTSIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OUTSIDE))
#define GEGL_OUTSIDE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OUTSIDE, GeglOutsideClass))

#ifndef __TYPEDEF_GEGL_OUTSIDE__
#define __TYPEDEF_GEGL_OUTSIDE__
typedef struct _GeglOutside GeglOutside;
#endif
struct _GeglOutside 
{
   GeglComp comp;

   /*< private >*/
};

typedef struct _GeglOutsideClass GeglOutsideClass;
struct _GeglOutsideClass 
{
   GeglCompClass comp_class;
};

GType           gegl_outside_get_type         (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
