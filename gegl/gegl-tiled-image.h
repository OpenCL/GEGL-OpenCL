#ifndef __GEGL_TILED_IMAGE_H__
#define __GEGL_TILED_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image.h"


#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif

#define GEGL_TYPE_TILED_IMAGE               (gegl_tiled_image_get_type ())
#define GEGL_TILED_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILED_IMAGE, GeglTiledImage))
#define GEGL_TILED_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILED_IMAGE, GeglTiledImageClass))
#define GEGL_IS_TILED_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILED_IMAGE))
#define GEGL_IS_TILED_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILED_IMAGE))
#define GEGL_TILED_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILED_IMAGE, GeglTiledImageClass))

#ifndef __TYPEDEF_GEGL_TILED_IMAGE__
#define __TYPEDEF_GEGL_TILED_IMAGE__
typedef struct _GeglTiledImage GeglTiledImage;
#endif
struct _GeglTiledImage 
{
    GeglImage image;

    /*< private >*/
};

typedef struct _GeglTiledImageClass GeglTiledImageClass;
struct _GeglTiledImageClass 
{
    GeglImageClass image_class;
};

GType           gegl_tiled_image_get_type     (void);

GeglTile*       gegl_tiled_image_get_tile   (GeglTiledImage * self);
void            gegl_tiled_image_set_tile   (GeglTiledImage * self, 
                                             GeglTile *tile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
