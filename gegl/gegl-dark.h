#ifndef __GEGL_DARK_H__
#define __GEGL_DARK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_DARK               (gegl_dark_get_type ())
#define GEGL_DARK(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DARK, GeglDark))
#define GEGL_DARK_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DARK, GeglDarkClass))
#define GEGL_IS_DARK(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DARK))
#define GEGL_IS_DARK_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DARK))
#define GEGL_DARK_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DARK, GeglDarkClass))

#ifndef __TYPEDEF_GEGL_DARK__
#define __TYPEDEF_GEGL_DARK__
typedef struct _GeglDark GeglDark;
#endif
struct _GeglDark {
   GeglPointOp __parent__;
   /*< private >*/
};

typedef struct _GeglDarkClass GeglDarkClass;
struct _GeglDarkClass {
   GeglPointOpClass __parent__;
};

GType            gegl_dark_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
