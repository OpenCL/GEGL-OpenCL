#ifndef __GEGL_MOCK_OPERATION_0_1_H__
#define __GEGL_MOCK_OPERATION_0_1_H__

#include "../gegl/gegl-operation.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_OPERATION_0_1               (gegl_mock_operation_0_1_get_type ())
#define GEGL_MOCK_OPERATION_0_1(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_OPERATION_0_1, GeglMockOperation01))
#define GEGL_MOCK_OPERATION_0_1_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_OPERATION_0_1, GeglMockOperation01Class))
#define GEGL_IS_MOCK_OPERATION_0_1(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_OPERATION_0_1))
#define GEGL_IS_MOCK_OPERATION_0_1_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_OPERATION_0_1))
#define GEGL_MOCK_OPERATION_0_1_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_OPERATION_0_1, GeglMockOperation01Class))

typedef struct _GeglMockOperation01  GeglMockOperation01;
struct _GeglMockOperation01
{
    GeglOperation       operation;

    /*< private >*/
    gint output0;
};

typedef struct _GeglMockOperation01Class GeglMockOperation01Class;
struct _GeglMockOperation01Class
{
   GeglOperationClass operation_class;
};

GType             gegl_mock_operation_0_1_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
