#ifndef __GEGL_MOCK_IMAGE_OP_H__
#define __GEGL_MOCK_IMAGE_OP_H__

#include "../gegl/gegl-image-op.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_IMAGE_OP               (gegl_mock_image_op_get_type ())
#define GEGL_MOCK_IMAGE_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_IMAGE_OP, GeglMockImageOp))
#define GEGL_MOCK_IMAGE_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_IMAGE_OP, GeglMockImageOpClass))
#define GEGL_IS_MOCK_IMAGE_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_IMAGE_OP))
#define GEGL_IS_MOCK_IMAGE_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_IMAGE_OP))
#define GEGL_MOCK_IMAGE_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_IMAGE_OP, GeglMockImageOpClass))

typedef struct _GeglMockImageOp  GeglMockImageOp;
struct _GeglMockImageOp 
{
    GeglImageOp       image_op;

    /*< private >*/
};

typedef struct _GeglMockImageOpClass GeglMockImageOpClass;
struct _GeglMockImageOpClass 
{
   GeglImageOpClass image_op_class;
};

GType             gegl_mock_image_op_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
