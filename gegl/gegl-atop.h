#ifndef __GEGL_ATOP_H__
#define __GEGL_ATOP_H__

#include "gegl-comp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_ATOP               (gegl_atop_get_type ())
#define GEGL_ATOP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_ATOP, GeglAtop))
#define GEGL_ATOP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_ATOP, GeglAtopClass))
#define GEGL_IS_ATOP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_ATOP))
#define GEGL_IS_ATOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_ATOP))
#define GEGL_ATOP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_ATOP, GeglAtopClass))

typedef struct _GeglAtop GeglAtop;
typedef struct _GeglAtopClass GeglAtopClass;

struct _GeglAtop 
{
   GeglComp comp;

   /*< private >*/
};

struct _GeglAtopClass 
{
   GeglCompClass comp_class;
};

GType           gegl_atop_get_type         (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
