#ifndef __GEGL_DARKEN_H__
#define __GEGL_DARKEN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-blend.h"

#define GEGL_TYPE_DARKEN               (gegl_darken_get_type ())
#define GEGL_DARKEN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DARKEN, GeglDarken))
#define GEGL_DARKEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DARKEN, GeglDarkenClass))
#define GEGL_IS_DARKEN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DARKEN))
#define GEGL_IS_DARKEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DARKEN))
#define GEGL_DARKEN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DARKEN, GeglDarkenClass))

#ifndef __TYPEDEF_GEGL_DARKEN__
#define __TYPEDEF_GEGL_DARKEN__
typedef struct _GeglDarken GeglDarken;
#endif
struct _GeglDarken 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglDarkenClass GeglDarkenClass;
struct _GeglDarkenClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_darken_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
