#ifndef __GEGL_MOCK_FILTER_1_2_H__
#define __GEGL_MOCK_FILTER_1_2_H__

#include "../gegl/gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_FILTER_1_2               (gegl_mock_filter_1_2_get_type ())
#define GEGL_MOCK_FILTER_1_2(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_FILTER_1_2, GeglMockFilter12))
#define GEGL_MOCK_FILTER_1_2_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_FILTER_1_2, GeglMockFilter12Class))
#define GEGL_IS_MOCK_FILTER_1_2(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_FILTER_1_2))
#define GEGL_IS_MOCK_FILTER_1_2_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_FILTER_1_2))
#define GEGL_MOCK_FILTER_1_2_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_FILTER_1_2, GeglMockFilter12Class))

typedef struct _GeglMockFilter12  GeglMockFilter12;
struct _GeglMockFilter12 
{
    GeglFilter       filter;

    /*< private >*/
    gint output0;
    gint output1;

    gint input0;
};

typedef struct _GeglMockFilter12Class GeglMockFilter12Class;
struct _GeglMockFilter12Class 
{
   GeglFilterClass op_class;
};

GType             gegl_mock_filter_1_2_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
