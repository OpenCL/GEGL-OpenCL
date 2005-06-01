#ifndef __GEGL_MOCK_IMAGE_H__
#define __GEGL_MOCK_IMAGE_H__

#include "../gegl/gegl-object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_IMAGE               (gegl_mock_image_get_type ())
#define GEGL_MOCK_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_IMAGE, GeglMockImage))
#define GEGL_MOCK_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_IMAGE, GeglMockImageClass))
#define GEGL_IS_MOCK_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_IMAGE))
#define GEGL_IS_MOCK_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_IMAGE))
#define GEGL_MOCK_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_IMAGE, GeglMockImageClass))

typedef struct _GeglMockImage GeglMockImage;

struct _GeglMockImage
{
  GeglObject object;

  /*< private >*/
  gint *data;
  gint length;
  gint default_pixel;
};

typedef struct _GeglMockImageClass GeglMockImageClass;
struct _GeglMockImageClass
{
  GeglObjectClass object_class;
};

GType           gegl_mock_image_get_type              (void);

gint * gegl_mock_image_get_data(GeglMockImage *self);
gint gegl_mock_image_get_length(GeglMockImage *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
