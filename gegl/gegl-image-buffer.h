#ifndef __GEGL_IMAGE_BUFFER_H__
#define __GEGL_IMAGE_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile  GeglTile;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_IMAGE_BUFFER               (gegl_image_buffer_get_type ())
#define GEGL_IMAGE_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_BUFFER, GeglImageBuffer))
#define GEGL_IMAGE_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_BUFFER, GeglImageBufferClass))
#define GEGL_IS_IMAGE_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_BUFFER))
#define GEGL_IS_IMAGE_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_BUFFER))
#define GEGL_IMAGE_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_BUFFER, GeglImageBufferClass))

#ifndef __TYPEDEF_GEGL_IMAGE_BUFFER__
#define __TYPEDEF_GEGL_IMAGE_BUFFER__
typedef struct _GeglImageBuffer GeglImageBuffer;
#endif
struct _GeglImageBuffer 
{
    GeglObject object;

    /*< private >*/
    GeglTile * tile;
    GeglColorModel * color_model;
};

typedef struct _GeglImageBufferClass GeglImageBufferClass;
struct _GeglImageBufferClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_buffer_get_type              (void);

GeglTile *    gegl_image_buffer_get_tile            (GeglImageBuffer * self);
void            gegl_image_buffer_set_tile            (GeglImageBuffer * self,
                                                       GeglTile *tile);
GeglColorModel*   gegl_image_buffer_get_color_model         (GeglImageBuffer * self);
void            gegl_image_buffer_set_color_model         (GeglImageBuffer * self, 
                                                       GeglColorModel * color_model);
void            gegl_image_buffer_create_tile         (GeglImageBuffer * self, 
                                                       GeglColorModel * color_model,
                                                       GeglRect * area);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
