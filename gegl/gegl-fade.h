#ifndef __GEGL_FADE_H__
#define __GEGL_FADE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-unary.h"

#define GEGL_TYPE_FADE               (gegl_fade_get_type ())
#define GEGL_FADE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FADE, GeglFade))
#define GEGL_FADE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FADE, GeglFadeClass))
#define GEGL_IS_FADE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FADE))
#define GEGL_IS_FADE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FADE))
#define GEGL_FADE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FADE, GeglFadeClass))

#ifndef __TYPEDEF_GEGL_FADE__
#define __TYPEDEF_GEGL_FADE__
typedef struct _GeglFade GeglFade;
#endif
struct _GeglFade 
{
   GeglUnary unary;
   /*< private >*/
   gfloat multiplier;
};

typedef struct _GeglFadeClass GeglFadeClass;
struct _GeglFadeClass 
{
   GeglUnaryClass unary_class;
};

GType            gegl_fade_get_type         (void);

gfloat          gegl_fade_get_multiplier     (GeglFade * self);
void            gegl_fade_set_multiplier     (GeglFade * self, 
                                              gfloat multiplier);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
