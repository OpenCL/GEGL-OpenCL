#ifndef __GEGL_LIGHTEN_H__
#define __GEGL_LIGHTEN_H__

#include "gegl-blend.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_LIGHTEN               (gegl_lighten_get_type ())
#define GEGL_LIGHTEN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_LIGHTEN, GeglLighten))
#define GEGL_LIGHTEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_LIGHTEN, GeglLightenClass))
#define GEGL_IS_LIGHTEN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_LIGHTEN))
#define GEGL_IS_LIGHTEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_LIGHTEN))
#define GEGL_LIGHTEN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_LIGHTEN, GeglLightenClass))

typedef struct _GeglLighten GeglLighten;
struct _GeglLighten 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglLightenClass GeglLightenClass;
struct _GeglLightenClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_lighten_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
