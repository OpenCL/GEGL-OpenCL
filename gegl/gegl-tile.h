#ifndef __GEGL_TILE_H__
#define __GEGL_TILE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_BUFFER__
#define __TYPEDEF_GEGL_BUFFER__
typedef struct _GeglBuffer  GeglBuffer;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_TILE               (gegl_tile_get_type ())
#define GEGL_TILE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE, GeglTile))
#define GEGL_TILE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE, GeglTileClass))
#define GEGL_IS_TILE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE))
#define GEGL_IS_TILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE))
#define GEGL_TILE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE, GeglTileClass))
#define G_VALUE_HOLDS_TILE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_TILE))

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif
struct _GeglTile 
{
    GeglObject object;

    /*< private >*/
    GeglBuffer * buffer;
    GeglColorModel * color_model;
    GeglRect area;
};

typedef struct _GeglTileClass GeglTileClass;
struct _GeglTileClass 
{
    GeglObjectClass object_class;
};

GType           gegl_tile_get_type              (void);
GeglBuffer *    gegl_tile_get_buffer            (GeglTile * self);
void            gegl_tile_get_area              (GeglTile * self,
                                                 GeglRect * area);
GeglColorModel* gegl_tile_get_color_model       (GeglTile * self);
void            gegl_tile_get_data              (GeglTile * self,
                                                 gpointer * data);
void            gegl_tile_get_data_at           (GeglTile * self,
                                                 gpointer * data,
                                                 gint x,
                                                 gint y);
gpointer*       gegl_tile_data_pointers         (GeglTile * self, 
                                                 gint x, 
                                                 gint y);

gboolean        gegl_tile_alloc                 (GeglTile * self,
                                                 GeglRect * area,
                                                 GeglColorModel * color_model);
void            gegl_tile_validate_data         (GeglTile * self);

void            gegl_tile_set_area              (GeglTile * self,
                                                 GeglRect * area);
void            gegl_tile_set_color_model       (GeglTile * self,
                                                 GeglColorModel * color_model);

void            g_value_set_tile                (GValue         *value,
                                                 gpointer        v_object);
gpointer        g_value_get_tile                (const GValue   *value);
GeglTile*       g_value_dup_tile                (const GValue   *value);
void            g_value_set_tile_take_ownership (GValue         *value,
                                                 gpointer        v_object);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
