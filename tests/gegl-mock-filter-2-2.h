#ifndef __GEGL_MOCK_FILTER_2_2_H__
#define __GEGL_MOCK_FILTER_2_2_H__

#include "../gegl/gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_FILTER_2_2               (gegl_mock_filter_2_2_get_type ())
#define GEGL_MOCK_FILTER_2_2(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_FILTER_2_2, GeglMockFilter22))
#define GEGL_MOCK_FILTER_2_2_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_FILTER_2_2, GeglMockFilter22Class))
#define GEGL_IS_MOCK_FILTER_2_2(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_FILTER_2_2))
#define GEGL_IS_MOCK_FILTER_2_2_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_FILTER_2_2))
#define GEGL_MOCK_FILTER_2_2_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_FILTER_2_2, GeglMockFilter22Class))

typedef struct _GeglMockFilter22  GeglMockFilter22;
struct _GeglMockFilter22 
{
    GeglFilter       filter;

    /*< private >*/
    gint output0;
    gint output1;

    gint input0;
    gint input1;
};

typedef struct _GeglMockFilter22Class GeglMockFilter22Class;
struct _GeglMockFilter22Class 
{
   GeglFilterClass filter_class;
};

GType             gegl_mock_filter_2_2_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
