#ifndef __GEGL_SCREEN_H__
#define __GEGL_SCREEN_H__

#include "gegl-blend.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_SCREEN               (gegl_screen_get_type ())
#define GEGL_SCREEN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SCREEN, GeglScreen))
#define GEGL_SCREEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SCREEN, GeglScreenClass))
#define GEGL_IS_SCREEN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SCREEN))
#define GEGL_IS_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SCREEN))
#define GEGL_SCREEN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SCREEN, GeglScreenClass))

typedef struct _GeglScreen GeglScreen;
struct _GeglScreen 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglScreenClass GeglScreenClass;
struct _GeglScreenClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_screen_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
