#ifndef __GEGL_LIGHT_H__
#define __GEGL_LIGHT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_LIGHT               (gegl_light_get_type ())
#define GEGL_LIGHT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_LIGHT, GeglLight))
#define GEGL_LIGHT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_LIGHT, GeglLightClass))
#define GEGL_IS_LIGHT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_LIGHT))
#define GEGL_IS_LIGHT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_LIGHT))
#define GEGL_LIGHT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_LIGHT, GeglLightClass))

#ifndef __TYPEDEF_GEGL_LIGHT__
#define __TYPEDEF_GEGL_LIGHT__
typedef struct _GeglLight GeglLight;
#endif
struct _GeglLight 
{
   GeglPointOp point_op;
   /*< private >*/
};

typedef struct _GeglLightClass GeglLightClass;
struct _GeglLightClass 
{
   GeglPointOpClass point_op_class;
};

GType            gegl_light_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
