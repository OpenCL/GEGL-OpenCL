#ifndef __GEGL_BINARY_H__
#define __GEGL_BINARY_H__

#include "gegl-point-op.h"
#include "gegl-scanline-processor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_BINARY               (gegl_binary_get_type ())
#define GEGL_BINARY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BINARY, GeglBinary))
#define GEGL_BINARY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BINARY, GeglBinaryClass))
#define GEGL_IS_BINARY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BINARY))
#define GEGL_IS_BINARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BINARY))
#define GEGL_BINARY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BINARY, GeglBinaryClass))

typedef struct _GeglBinary GeglBinary;
struct _GeglBinary 
{
    GeglPointOp point_op;

    /*< private >*/
};

typedef struct _GeglBinaryClass GeglBinaryClass;
struct _GeglBinaryClass 
{
    GeglPointOpClass point_op_class;

    GeglScanlineFunc (*get_scanline_func)   (GeglBinary *self,
                                             GeglColorSpaceType space,
                                             GeglChannelSpaceType type);
};

GType           gegl_binary_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
