#ifndef __GEGL_UNARY_H__
#define __GEGL_UNARY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"
#include "gegl-scanline-processor.h"

#define GEGL_TYPE_UNARY               (gegl_unary_get_type ())
#define GEGL_UNARY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_UNARY, GeglUnary))
#define GEGL_UNARY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_UNARY, GeglUnaryClass))
#define GEGL_IS_UNARY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_UNARY))
#define GEGL_IS_UNARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_UNARY))
#define GEGL_UNARY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_UNARY, GeglUnaryClass))

#ifndef __TYPEDEF_GEGL_UNARY__
#define __TYPEDEF_GEGL_UNARY__
typedef struct _GeglUnary GeglUnary;
#endif
struct _GeglUnary 
{
    GeglPointOp point_op;

    /*< private >*/
};

typedef struct _GeglUnaryClass GeglUnaryClass;
struct _GeglUnaryClass 
{
    GeglPointOpClass point_op_class;

    GeglScanlineFunc (*get_scanline_func)   (GeglUnary *self,
                                             GeglColorSpace space,
                                             GeglChannelDataType type);
};

GType           gegl_unary_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
