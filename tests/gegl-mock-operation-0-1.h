#ifndef __GEGL_MOCK_FILTER_0_1_H__
#define __GEGL_MOCK_FILTER_0_1_H__

#include "../gegl/gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_FILTER_0_1               (gegl_mock_filter_0_1_get_type ())
#define GEGL_MOCK_FILTER_0_1(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_FILTER_0_1, GeglMockFilter01))
#define GEGL_MOCK_FILTER_0_1_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_FILTER_0_1, GeglMockFilter01Class))
#define GEGL_IS_MOCK_FILTER_0_1(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_FILTER_0_1))
#define GEGL_IS_MOCK_FILTER_0_1_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_FILTER_0_1))
#define GEGL_MOCK_FILTER_0_1_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_FILTER_0_1, GeglMockFilter01Class))

typedef struct _GeglMockFilter01  GeglMockFilter01;
struct _GeglMockFilter01 
{
    GeglFilter       filter;

    /*< private >*/
    gint output0;
};

typedef struct _GeglMockFilter01Class GeglMockFilter01Class;
struct _GeglMockFilter01Class 
{
   GeglFilterClass filter_class;
};

GType             gegl_mock_filter_0_1_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
