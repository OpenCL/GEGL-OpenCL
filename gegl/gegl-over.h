#ifndef __GEGL_OVER_H__
#define __GEGL_OVER_H__

#include "gegl-comp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_OVER               (gegl_over_get_type ())
#define GEGL_OVER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OVER, GeglOver))
#define GEGL_OVER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OVER, GeglOverClass))
#define GEGL_IS_OVER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OVER))
#define GEGL_IS_OVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OVER))
#define GEGL_OVER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OVER, GeglOverClass))

typedef struct _GeglOver GeglOver;
struct _GeglOver
{
   GeglComp comp;

   /*< private >*/
};

typedef struct _GeglOverClass GeglOverClass;
struct _GeglOverClass
{
   GeglCompClass comp_class;
};

GType           gegl_over_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
