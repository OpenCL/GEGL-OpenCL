#ifndef __GEGL_BLEND_H__
#define __GEGL_BLEND_H__

#include "gegl-comp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_BLEND               (gegl_blend_get_type ())
#define GEGL_BLEND(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BLEND, GeglBlend))
#define GEGL_BLEND_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BLEND, GeglBlendClass))
#define GEGL_IS_BLEND(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BLEND))
#define GEGL_IS_BLEND_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BLEND))
#define GEGL_BLEND_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BLEND, GeglBlendClass))

typedef struct _GeglBlend GeglBlend;
struct _GeglBlend 
{
    GeglComp comp;

    /*< private >*/
};

typedef struct _GeglBlendClass GeglBlendClass;
struct _GeglBlendClass 
{
    GeglCompClass comp_class;
};

GType           gegl_blend_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
