#ifndef __GEGL_MOCK_OP_H__
#define __GEGL_MOCK_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op.h"

#define GEGL_TYPE_MOCK_OP               (gegl_mock_op_get_type ())
#define GEGL_MOCK_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_OP, GeglMockOp))
#define GEGL_MOCK_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_OP, GeglMockOpClass))
#define GEGL_IS_MOCK_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_OP))
#define GEGL_IS_MOCK_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_OP))
#define GEGL_MOCK_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_OP, GeglMockOpClass))

#ifndef __TYPEDEF_GEGL_MOCK_OP__
#define __TYPEDEF_GEGL_MOCK_OP__
typedef struct _GeglMockOp GeglMockOp;
#endif
struct _GeglMockOp {
       GeglOp __parent__;
};

typedef struct _GeglMockOpClass GeglMockOpClass;
struct _GeglMockOpClass {
       GeglOpClass __parent__;
};

GType         gegl_mock_op_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
