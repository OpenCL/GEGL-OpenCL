#ifndef __GEGL_DIFF_H__
#define __GEGL_DIFF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_DIFF               (gegl_diff_get_type ())
#define GEGL_DIFF(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DIFF, GeglDiff))
#define GEGL_DIFF_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DIFF, GeglDiffClass))
#define GEGL_IS_DIFF(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DIFF))
#define GEGL_IS_DIFF_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DIFF))
#define GEGL_DIFF_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DIFF, GeglDiffClass))

#ifndef __TYPEDEF_GEGL_DIFF__
#define __TYPEDEF_GEGL_DIFF__
typedef struct _GeglDiff GeglDiff;
#endif
struct _GeglDiff {
   GeglPointOp __parent__;
   /*< private >*/
};

typedef struct _GeglDiffClass GeglDiffClass;
struct _GeglDiffClass {
   GeglPointOpClass __parent__;
};

GType            gegl_diff_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
