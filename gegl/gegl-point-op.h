#ifndef __GEGL_POINT_OP_H__
#define __GEGL_POINT_OP_H__

#include "gegl-image-op.h"
#include "gegl-scanline-processor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_POINT_OP               (gegl_point_op_get_type ())
#define GEGL_POINT_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_POINT_OP, GeglPointOp))
#define GEGL_POINT_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_POINT_OP, GeglPointOpClass))
#define GEGL_IS_POINT_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_POINT_OP))
#define GEGL_IS_POINT_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_POINT_OP))
#define GEGL_POINT_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_POINT_OP, GeglPointOpClass))

typedef struct _GeglPointOp GeglPointOp;
struct _GeglPointOp 
{
    GeglImageOp image_op;

    /*< private >*/
    GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglPointOpClass GeglPointOpClass;
struct _GeglPointOpClass 
{
    GeglImageOpClass image_op_class;
};

GType           gegl_point_op_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
