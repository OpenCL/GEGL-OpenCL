#ifndef __GEGL_MOCK_POINT_OP_H__
#define __GEGL_MOCK_POINT_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../gegl/gegl-point-op.h"

#define GEGL_TYPE_MOCK_POINT_OP               (gegl_mock_point_op_get_type ())
#define GEGL_MOCK_POINT_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_POINT_OP, GeglMockPointOp))
#define GEGL_MOCK_POINT_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_POINT_OP, GeglMockPointOpClass))
#define GEGL_IS_MOCK_POINT_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_POINT_OP))
#define GEGL_IS_MOCK_POINT_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_POINT_OP))
#define GEGL_MOCK_POINT_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_POINT_OP, GeglMockPointOpClass))

#ifndef __TYPEDEF_GEGL_MOCK_POINT_OP__
#define __TYPEDEF_GEGL_MOCK_POINT_OP__
typedef struct _GeglMockPointOp  GeglMockPointOp;
#endif

struct _GeglMockPointOp 
{
    GeglPointOp    point_op;

    /*< private >*/
};

typedef struct _GeglMockPointOpClass GeglMockPointOpClass;
struct _GeglMockPointOpClass 
{
   GeglPointOpClass point_op_class;
};

GType             gegl_mock_point_op_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
