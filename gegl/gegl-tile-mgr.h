#ifndef __GEGL_TILE_MGR_H__
#define __GEGL_TILE_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif

#ifndef __TYPEDEF_GEGL_BUFFER__
#define __TYPEDEF_GEGL_BUFFER__
typedef struct _GeglBuffer GeglBuffer;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_TILE_MGR               (gegl_tile_mgr_get_type ())
#define GEGL_TILE_MGR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_MGR, GeglTileMgr))
#define GEGL_TILE_MGR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_MGR, GeglTileMgrClass))
#define GEGL_IS_TILE_MGR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_MGR))
#define GEGL_IS_TILE_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_MGR))
#define GEGL_TILE_MGR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_MGR, GeglTileMgrClass))

#ifndef __TYPEDEF_GEGL_TILE_MGR__
#define __TYPEDEF_GEGL_TILE_MGR__
typedef struct _GeglTileMgr GeglTileMgr;
#endif
struct _GeglTileMgr 
{
    GeglObject object;

    /*< private >*/
};

typedef struct _GeglTileMgrClass GeglTileMgrClass;
struct _GeglTileMgrClass 
{
    GeglObjectClass object_class;
};

GType           gegl_tile_mgr_get_type          (void);
GeglTileMgr *   gegl_tile_mgr_instance          (void);
void            gegl_tile_mgr_install           (GeglTileMgr *tile_mgr);

GeglTile *      gegl_tile_mgr_validate_tile     (GeglTileMgr * self, 
                                                 GeglTile * tile, 
                                                 GeglRect * area,
                                                 GeglColorModel * color_model);
void            gegl_tile_mgr_validate_data     (GeglTileMgr * self, 
                                                 GeglTile * tile);
GeglBuffer *    gegl_tile_mgr_create_buffer     (GeglTileMgr * self, 
                                                 GeglTile * tile);
GeglTile *      gegl_tile_mgr_create_tile       (GeglTileMgr * self, 
                                                 GeglColorModel * color_model, 
                                                 GeglRect * area);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
