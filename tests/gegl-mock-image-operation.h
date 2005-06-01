#ifndef __GEGL_MOCK_IMAGE_FILTER_H__
#define __GEGL_MOCK_IMAGE_FILTER_H__

#include "../gegl/gegl-filter.h"
#include "gegl-mock-image.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_IMAGE_FILTER               (gegl_mock_image_filter_get_type ())
#define GEGL_MOCK_IMAGE_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_IMAGE_FILTER, GeglMockImageFilter))
#define GEGL_MOCK_IMAGE_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_IMAGE_FILTER, GeglMockImageFilterClass))
#define GEGL_IS_MOCK_IMAGE_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_IMAGE_FILTER))
#define GEGL_IS_MOCK_IMAGE_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_IMAGE_FILTER))
#define GEGL_MOCK_IMAGE_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_IMAGE_FILTER, GeglMockImageFilterClass))

typedef struct _GeglMockImageFilter  GeglMockImageFilter;
struct _GeglMockImageFilter
{
    GeglFilter       filter;

    /*< private >*/
    GeglMockImage *output;
    GeglMockImage *input0;
    gint input1;
};

typedef struct _GeglMockImageFilterClass GeglMockImageFilterClass;
struct _GeglMockImageFilterClass
{
   GeglFilterClass filter_class;
};

GType             gegl_mock_image_filter_get_type                  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
