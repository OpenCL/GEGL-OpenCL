#ifndef __GEGL_NORMAL_H__
#define __GEGL_NORMAL_H__

#include "gegl-blend.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_NORMAL               (gegl_normal_get_type ())
#define GEGL_NORMAL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NORMAL, GeglNormal))
#define GEGL_NORMAL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NORMAL, GeglNormalClass))
#define GEGL_IS_NORMAL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NORMAL))
#define GEGL_IS_NORMAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NORMAL))
#define GEGL_NORMAL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NORMAL, GeglNormalClass))

typedef struct _GeglNormal GeglNormal;
struct _GeglNormal 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglNormalClass GeglNormalClass;
struct _GeglNormalClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_normal_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
