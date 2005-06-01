#ifndef __GEGL_MOCK_FILTER_2_1_H__
#define __GEGL_MOCK_FILTER_2_1_H__

#include "../gegl/gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_FILTER_2_1               (gegl_mock_filter_2_1_get_type ())
#define GEGL_MOCK_FILTER_2_1(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_FILTER_2_1, GeglMockFilter21))
#define GEGL_MOCK_FILTER_2_1_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_FILTER_2_1, GeglMockFilter21Class))
#define GEGL_IS_MOCK_FILTER_2_1(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_FILTER_2_1))
#define GEGL_IS_MOCK_FILTER_2_1_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_FILTER_2_1))
#define GEGL_MOCK_FILTER_2_1_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_FILTER_2_1, GeglMockFilter21Class))

typedef struct _GeglMockFilter21  GeglMockFilter21;
struct _GeglMockFilter21
{
    GeglFilter       filter;

    /*< private >*/
    gint output0;

    gint input0;
    gint input1;
};

typedef struct _GeglMockFilter21Class GeglMockFilter21Class;
struct _GeglMockFilter21Class
{
   GeglFilterClass filter_class;
};

GType             gegl_mock_filter_2_1_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
