#ifndef __GEGL_MOCK_IMAGE_OPERATION_H__
#define __GEGL_MOCK_IMAGE_OPERATION_H__

#include "../gegl/gegl-operation.h"
#include "gegl-mock-image.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_IMAGE_OPERATION               (gegl_mock_image_operation_get_type ())
#define GEGL_MOCK_IMAGE_OPERATION(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_IMAGE_OPERATION, GeglMockImageOperation))
#define GEGL_MOCK_IMAGE_OPERATION_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_IMAGE_OPERATION, GeglMockImageOperationClass))
#define GEGL_IS_MOCK_IMAGE_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_IMAGE_OPERATION))
#define GEGL_IS_MOCK_IMAGE_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_IMAGE_OPERATION))
#define GEGL_MOCK_IMAGE_OPERATION_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_IMAGE_OPERATION, GeglMockImageOperationClass))

typedef struct _GeglMockImageOperation  GeglMockImageOperation;
struct _GeglMockImageOperation
{
    GeglOperation       operation;

    /*< private >*/
    GeglMockImage *output;
    GeglMockImage *input0;
    gint input1;
};

typedef struct _GeglMockImageOperationClass GeglMockImageOperationClass;
struct _GeglMockImageOperationClass
{
   GeglOperationClass operation_class;
};

GType             gegl_mock_image_operation_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
