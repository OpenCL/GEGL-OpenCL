#ifndef __GEGL_STAT_OP_H__
#define __GEGL_STAT_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-filter.h"

#ifndef __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
#define __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
typedef struct _GeglScanlineProcessor  GeglScanlineProcessor;
#endif

#define GEGL_TYPE_STAT_OP               (gegl_stat_op_get_type ())
#define GEGL_STAT_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_STAT_OP, GeglStatOp))
#define GEGL_STAT_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STAT_OP, GeglStatOpClass))
#define GEGL_IS_STAT_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_STAT_OP))
#define GEGL_IS_STAT_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STAT_OP))
#define GEGL_STAT_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STAT_OP, GeglStatOpClass))

#ifndef __TYPEDEF_GEGL_STAT_OP__
#define __TYPEDEF_GEGL_STAT_OP__
typedef struct _GeglStatOp GeglStatOp;
#endif
struct _GeglStatOp 
{
    GeglFilter filter;

    /*< private >*/
    GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglStatOpClass GeglStatOpClass;
struct _GeglStatOpClass 
{
    GeglFilterClass filter_class;
};

GType           gegl_stat_op_get_type           (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
