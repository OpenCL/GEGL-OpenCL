#ifndef __GEGL_MOCK_IMAGE_H__
#define __GEGL_MOCK_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../gegl/gegl-image.h"

#define GEGL_TYPE_MOCK_IMAGE               (gegl_mock_image_get_type ())
#define GEGL_MOCK_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_IMAGE, GeglMockImage))
#define GEGL_MOCK_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_IMAGE, GeglMockImageClass))
#define GEGL_IS_MOCK_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_IMAGE))
#define GEGL_IS_MOCK_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_IMAGE))
#define GEGL_MOCK_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_IMAGE, GeglMockImageClass))

#ifndef __TYPEDEF_GEGL_MOCK_IMAGE__
#define __TYPEDEF_GEGL_MOCK_IMAGE__
typedef struct _GeglMockImage  GeglMockImage;
#endif

struct _GeglMockImage 
{
    GeglImage       __parent__;

    /*< private >*/
};

typedef struct _GeglMockImageClass GeglMockImageClass;
struct _GeglMockImageClass 
{
   GeglImageClass __parent__;
};

GType             gegl_mock_image_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
