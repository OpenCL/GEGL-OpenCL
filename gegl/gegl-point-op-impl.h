#ifndef __GEGL_POINT_OP_IMPL_H__
#define __GEGL_POINT_OP_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image-impl.h"

#ifndef __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
#define __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
typedef struct _GeglScanlineProcessor  GeglScanlineProcessor;
#endif

#define GEGL_TYPE_POINT_OP_IMPL               (gegl_point_op_impl_get_type ())
#define GEGL_POINT_OP_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_POINT_OP_IMPL, GeglPointOpImpl))
#define GEGL_POINT_OP_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_POINT_OP_IMPL, GeglPointOpImplClass))
#define GEGL_IS_POINT_OP_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_POINT_OP_IMPL))
#define GEGL_IS_POINT_OP_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_POINT_OP_IMPL))
#define GEGL_POINT_OP_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_POINT_OP_IMPL, GeglPointOpImplClass))

#ifndef __TYPEDEF_GEGL_POINT_OP_IMPL__
#define __TYPEDEF_GEGL_POINT_OP_IMPL__
typedef struct _GeglPointOpImpl GeglPointOpImpl;
#endif
struct _GeglPointOpImpl {
   GeglImageImpl __parent__;

   /*< private >*/
   GeglPoint * input_offsets;
   GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglPointOpImplClass GeglPointOpImplClass;
struct _GeglPointOpImplClass {
   GeglImageImplClass __parent__;
};

GType gegl_point_op_impl_get_type   (void);

void  gegl_point_op_impl_get_nth_input_offset  (GeglPointOpImpl * self, 
                                               gint i,
                                               GeglPoint *point);
void  gegl_point_op_impl_set_nth_input_offset  (GeglPointOpImpl * self, 
                                               gint i,
                                               GeglPoint * point);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
