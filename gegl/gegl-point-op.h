#ifndef __GEGL_POINT_OP_H__
#define __GEGL_POINT_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image.h"

#ifndef __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
#define __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
typedef struct _GeglScanlineProcessor  GeglScanlineProcessor;
#endif

#define GEGL_TYPE_POINT_OP               (gegl_point_op_get_type ())
#define GEGL_POINT_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_POINT_OP, GeglPointOp))
#define GEGL_POINT_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_POINT_OP, GeglPointOpClass))
#define GEGL_IS_POINT_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_POINT_OP))
#define GEGL_IS_POINT_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_POINT_OP))
#define GEGL_POINT_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_POINT_OP, GeglPointOpClass))

#ifndef __TYPEDEF_GEGL_POINT_OP__
#define __TYPEDEF_GEGL_POINT_OP__
typedef struct _GeglPointOp GeglPointOp;
#endif
struct _GeglPointOp 
{
   GeglImage __parent__;

   /*< private >*/
   GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglPointOpClass GeglPointOpClass;
struct _GeglPointOpClass 
{
   GeglImageClass __parent__;
};

GType            gegl_point_op_get_type              (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
