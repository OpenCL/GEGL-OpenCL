#ifndef __GEGL_TILE_H__
#define __GEGL_TILE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_DATA_BUFFER__
#define __TYPEDEF_GEGL_DATA_BUFFER__
typedef struct _GeglDataBuffer  GeglDataBuffer;
#endif

#ifndef __TYPEDEF_GEGL_STORAGE__
#define __TYPEDEF_GEGL_STORAGE__
typedef struct _GeglStorage  GeglStorage;
#endif

#define GEGL_TYPE_TILE               (gegl_tile_get_type ())
#define GEGL_TILE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE, GeglTile))
#define GEGL_TILE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE, GeglTileClass))
#define GEGL_IS_TILE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE))
#define GEGL_IS_TILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE))
#define GEGL_TILE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE, GeglTileClass))

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif
struct _GeglTile 
{
    GeglObject object;

    /*< private >*/
    GeglRect area;

    GeglDataBuffer * data_buffer;
    GeglStorage * storage;
};

typedef struct _GeglTileClass GeglTileClass;
struct _GeglTileClass 
{
    GeglObjectClass object_class;
};

GType           gegl_tile_get_type              (void);
GeglDataBuffer* gegl_tile_get_data_buffer       (GeglTile * self);
void            gegl_tile_get_area              (GeglTile * self,
                                                   GeglRect * area);
GeglStorage*    gegl_tile_get_storage           (GeglTile * self);
gpointer*       gegl_tile_data_pointers         (GeglTile * self, 
                                                   gint x, 
                                                   gint y);
gint            gegl_tile_get_x                 (GeglTile * self);
gint            gegl_tile_get_y                 (GeglTile * self);
gint            gegl_tile_get_width             (GeglTile * self);
gint            gegl_tile_get_height            (GeglTile * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
