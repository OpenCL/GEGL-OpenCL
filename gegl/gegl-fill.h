#ifndef __GEGL_FILL_H__
#define __GEGL_FILL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor  GeglColor;
#endif


#define GEGL_TYPE_FILL               (gegl_fill_get_type ())
#define GEGL_FILL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILL, GeglFill))
#define GEGL_FILL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILL, GeglFillClass))
#define GEGL_IS_FILL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILL))
#define GEGL_IS_FILL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILL))
#define GEGL_FILL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILL, GeglFillClass))

#ifndef __TYPEDEF_GEGL_FILL__
#define __TYPEDEF_GEGL_FILL__
typedef struct _GeglFill GeglFill;
#endif
struct _GeglFill {
   GeglPointOp __parent__;

   /*< private >*/
};

typedef struct _GeglFillClass GeglFillClass;
struct _GeglFillClass {
   GeglPointOpClass __parent__;
};

GType       gegl_fill_get_type       (void);
GeglColor * gegl_fill_get_fill_color (GeglFill * self);
void        gegl_fill_set_fill_color (GeglFill * self, 
                                         GeglColor *fill_color);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
