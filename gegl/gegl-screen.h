#ifndef __GEGL_SCREEN_H__
#define __GEGL_SCREEN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_SCREEN               (gegl_screen_get_type ())
#define GEGL_SCREEN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SCREEN, GeglScreen))
#define GEGL_SCREEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SCREEN, GeglScreenClass))
#define GEGL_IS_SCREEN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SCREEN))
#define GEGL_IS_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SCREEN))
#define GEGL_SCREEN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SCREEN, GeglScreenClass))

#ifndef __TYPEDEF_GEGL_SCREEN__
#define __TYPEDEF_GEGL_SCREEN__
typedef struct _GeglScreen GeglScreen;
#endif
struct _GeglScreen {
   GeglPointOp __parent__;
   /*< private >*/
};

typedef struct _GeglScreenClass GeglScreenClass;
struct _GeglScreenClass {
   GeglPointOpClass __parent__;
};

GType            gegl_screen_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
