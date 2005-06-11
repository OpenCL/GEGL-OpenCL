#ifndef __GEGL_MOCK_OPERATION_2_2_H__
#define __GEGL_MOCK_OPERATION_2_2_H__

#include "../gegl/gegl-operation.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_OPERATION_2_2               (gegl_mock_operation_2_2_get_type ())
#define GEGL_MOCK_OPERATION_2_2(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_OPERATION_2_2, GeglMockOperation22))
#define GEGL_MOCK_OPERATION_2_2_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_OPERATION_2_2, GeglMockOperation22Class))
#define GEGL_IS_MOCK_OPERATION_2_2(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_OPERATION_2_2))
#define GEGL_IS_MOCK_OPERATION_2_2_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_OPERATION_2_2))
#define GEGL_MOCK_OPERATION_2_2_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_OPERATION_2_2, GeglMockOperation22Class))

typedef struct _GeglMockOperation22  GeglMockOperation22;
struct _GeglMockOperation22
{
    GeglOperation       operation;

    /*< private >*/
    gint output0;
    gint output1;

    gint input0;
    gint input1;
};

typedef struct _GeglMockOperation22Class GeglMockOperation22Class;
struct _GeglMockOperation22Class
{
   GeglOperationClass operation_class;
};

GType             gegl_mock_operation_2_2_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
