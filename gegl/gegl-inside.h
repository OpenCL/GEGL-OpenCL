#ifndef __GEGL_INSIDE_H__
#define __GEGL_INSIDE_H__

#include "gegl-comp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_INSIDE               (gegl_inside_get_type ())
#define GEGL_INSIDE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_INSIDE, GeglInside))
#define GEGL_INSIDE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_INSIDE, GeglInsideClass))
#define GEGL_IS_INSIDE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_INSIDE))
#define GEGL_IS_INSIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_INSIDE))
#define GEGL_INSIDE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_INSIDE, GeglInsideClass))

typedef struct _GeglInside GeglInside;
struct _GeglInside 
{
   GeglComp comp;

   /*< private >*/
};

typedef struct _GeglInsideClass GeglInsideClass;
struct _GeglInsideClass 
{
   GeglCompClass comp_class;
};

GType           gegl_inside_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
