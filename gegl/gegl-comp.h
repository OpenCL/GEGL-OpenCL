#ifndef __GEGL_COMP_H__
#define __GEGL_COMP_H__

#include "gegl-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COMP               (gegl_comp_get_type ())
#define GEGL_COMP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMP, GeglComp))
#define GEGL_COMP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMP, GeglCompClass))
#define GEGL_IS_COMP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMP))
#define GEGL_IS_COMP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMP))
#define GEGL_COMP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMP, GeglCompClass))

typedef struct _GeglComp GeglComp;
struct _GeglComp
{
    GeglPointOp point_op;

    /*< private >*/
};

typedef struct _GeglCompClass GeglCompClass;
struct _GeglCompClass
{
    GeglPointOpClass point_op_class;

    GeglScanlineFunc (*get_scanline_function)(GeglComp *self,
                                              GeglColorModel *cm);
};

GType           gegl_comp_get_type          (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
