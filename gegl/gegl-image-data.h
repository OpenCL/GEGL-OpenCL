#ifndef __GEGL_IMAGE_DATA_H__
#define __GEGL_IMAGE_DATA_H__

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

#define GEGL_TYPE_IMAGE_DATA               (gegl_image_data_get_type ())
#define GEGL_IMAGE_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_DATA, GeglImageData))
#define GEGL_IMAGE_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_DATA, GeglImageDataClass))
#define GEGL_IS_IMAGE_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_DATA))
#define GEGL_IS_IMAGE_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_DATA))
#define GEGL_IMAGE_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_DATA, GeglImageDataClass))

#ifndef __TYPEDEF_GEGL_IMAGE_DATA__
#define __TYPEDEF_GEGL_IMAGE_DATA__
typedef struct _GeglImageData GeglImageData;
#endif
struct _GeglImageData 
{
    GeglObject object;

    /*< private >*/
    GeglTile * tile;
    GeglColorModel * color_model;
};

typedef struct _GeglImageDataClass GeglImageDataClass;
struct _GeglImageDataClass 
{
    GeglObjectClass object_class;
};

GType           gegl_image_data_get_type              (void);

GeglTile *    gegl_image_data_get_tile            (GeglImageData * self);
void            gegl_image_data_set_tile            (GeglImageData * self,
                                                       GeglTile *tile);
GeglColorModel*   gegl_image_data_get_color_model         (GeglImageData * self);
void            gegl_image_data_set_color_model         (GeglImageData * self, 
                                                       GeglColorModel * color_model);
void            gegl_image_data_create_tile         (GeglImageData * self, 
                                                       GeglColorModel * color_model,
                                                       GeglRect * area);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
