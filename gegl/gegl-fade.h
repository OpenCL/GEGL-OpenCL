#ifndef __GEGL_FADE_H__
#define __GEGL_FADE_H__

#include "gegl-unary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_FADE               (gegl_fade_get_type ())
#define GEGL_FADE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FADE, GeglFade))
#define GEGL_FADE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FADE, GeglFadeClass))
#define GEGL_IS_FADE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FADE))
#define GEGL_IS_FADE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FADE))
#define GEGL_FADE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FADE, GeglFadeClass))

typedef struct _GeglFade GeglFade;
struct _GeglFade
{
   GeglUnary unary;

   /*< private >*/
};

typedef struct _GeglFadeClass GeglFadeClass;
struct _GeglFadeClass
{
   GeglUnaryClass unary_class;
};

GType            gegl_fade_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
