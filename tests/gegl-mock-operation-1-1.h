#ifndef __GEGL_MOCK_OPERATION_1_1_H__
#define __GEGL_MOCK_OPERATION_1_1_H__

#include "../gegl/gegl-operation.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_OPERATION_1_1               (gegl_mock_operation_1_1_get_type ())
#define GEGL_MOCK_OPERATION_1_1(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_OPERATION_1_1, GeglMockOperation11))
#define GEGL_MOCK_OPERATION_1_1_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_OPERATION_1_1, GeglMockOperation11Class))
#define GEGL_IS_MOCK_OPERATION_1_1(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_OPERATION_1_1))
#define GEGL_IS_MOCK_OPERATION_1_1_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_OPERATION_1_1))
#define GEGL_MOCK_OPERATION_1_1_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_OPERATION_1_1, GeglMockOperation11Class))

typedef struct _GeglMockOperation11  GeglMockOperation11;
struct _GeglMockOperation11
{
    GeglOperation       operation;

    /*< private >*/
    gint output0;
    gint input0;
};

typedef struct _GeglMockOperation11Class GeglMockOperation11Class;
struct _GeglMockOperation11Class
{
   GeglOperationClass operation_class;
};

GType             gegl_mock_operation_1_1_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
