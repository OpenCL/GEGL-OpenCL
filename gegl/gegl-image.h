#ifndef __GEGL_IMAGE_H__
#define __GEGL_IMAGE_H__

#include "gegl-object.h"
#include "gegl-tile.h"
#include "gegl-color-model.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_IMAGE               (gegl_image_get_type ())
#define GEGL_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE, GeglImage))
#define GEGL_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE, GeglImageClass))
#define GEGL_IS_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE))
#define GEGL_IS_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE))
#define GEGL_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE, GeglImageClass))

typedef struct _GeglImage GeglImage;
struct _GeglImage 
{
    GeglObject object;

    /*< private >*/
    GeglTile * tile;
    GeglColorModel * color_model;
};

typedef struct _GeglImageClass GeglImageClass;
struct _GeglImageClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_get_type              (void);

GeglTile *      gegl_image_get_tile              (GeglImage * self);
void            gegl_image_set_tile              (GeglImage * self,
                                                  GeglTile *tile);
GeglColorModel* gegl_image_get_color_model       (GeglImage * self);
void            gegl_image_set_color_model       (GeglImage * self, 
                                                  GeglColorModel * color_model);
void            gegl_image_create_tile           (GeglImage * self, 
                                                  GeglColorModel * color_model,
                                                  GeglRect * area);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
