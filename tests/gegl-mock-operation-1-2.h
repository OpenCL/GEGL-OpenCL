#ifndef __GEGL_MOCK_OPERATION_1_2_H__
#define __GEGL_MOCK_OPERATION_1_2_H__

#include "../gegl/gegl-operation.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_OPERATION_1_2               (gegl_mock_operation_1_2_get_type ())
#define GEGL_MOCK_OPERATION_1_2(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_OPERATION_1_2, GeglMockOperation12))
#define GEGL_MOCK_OPERATION_1_2_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_OPERATION_1_2, GeglMockOperation12Class))
#define GEGL_IS_MOCK_OPERATION_1_2(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_OPERATION_1_2))
#define GEGL_IS_MOCK_OPERATION_1_2_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_OPERATION_1_2))
#define GEGL_MOCK_OPERATION_1_2_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_OPERATION_1_2, GeglMockOperation12Class))

typedef struct _GeglMockOperation12  GeglMockOperation12;
struct _GeglMockOperation12
{
    GeglOperation       operation;

    /*< private >*/
    gint output0;
    gint output1;

    gint input0;
};

typedef struct _GeglMockOperation12Class GeglMockOperation12Class;
struct _GeglMockOperation12Class
{
   GeglOperationClass op_class;
};

GType             gegl_mock_operation_1_2_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
