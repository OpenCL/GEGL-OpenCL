#ifndef __GEGL_STAT_OP_IMPL_H__
#define __GEGL_STAT_OP_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op-impl.h"

#ifndef __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
#define __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
typedef struct _GeglScanlineProcessor  GeglScanlineProcessor;
#endif

#define GEGL_TYPE_STAT_OP_IMPL               (gegl_stat_op_impl_get_type ())
#define GEGL_STAT_OP_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_STAT_OP_IMPL, GeglStatOpImpl))
#define GEGL_STAT_OP_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STAT_OP_IMPL, GeglStatOpImplClass))
#define GEGL_IS_STAT_OP_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_STAT_OP_IMPL))
#define GEGL_IS_STAT_OP_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STAT_OP_IMPL))
#define GEGL_STAT_OP_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STAT_OP_IMPL, GeglStatOpImplClass))

#ifndef __TYPEDEF_GEGL_STAT_OP_IMPL__
#define __TYPEDEF_GEGL_STAT_OP_IMPL__
typedef struct _GeglStatOpImpl GeglStatOpImpl;
#endif
struct _GeglStatOpImpl {
   GeglOpImpl __parent__;

   /*< private >*/
   GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglStatOpImplClass GeglStatOpImplClass;
struct _GeglStatOpImplClass {
   GeglOpImplClass __parent__;
};

GType gegl_stat_op_impl_get_type   (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
