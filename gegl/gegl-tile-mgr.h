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

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif

#ifndef __TYPEDEF_GEGL_BUFFER__
#define __TYPEDEF_GEGL_BUFFER__
typedef struct _GeglBuffer GeglBuffer;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE__
#define __TYPEDEF_GEGL_IMAGE__
typedef struct _GeglImage GeglImage;
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
struct _GeglTileMgr {
   GeglObject __parent__;

   /*< private >*/
};

typedef struct _GeglTileMgrClass GeglTileMgrClass;
struct _GeglTileMgrClass {
   GeglObjectClass __parent__;

   GeglTile * (* fetch_output_tile)  (GeglTileMgr * self,  
                                      GeglOp * op, 
                                      GeglImage * dest, 
                                      GeglRect * area);
   GeglTile * (* fetch_input_tile)   (GeglTileMgr * self, 
                                      GeglImage * input, 
                                      GeglRect * area);
   void       (* release_tiles)       (GeglTileMgr * self, 
                                       GList * request_list);
   GeglTile * (* get_tile)           (GeglTileMgr * self, 
                                      GeglImage* image);
   GeglBuffer * (* create_buffer)    (GeglTileMgr * self, 
                                      GeglTile * tile);
   GeglTile * (*make_tile)           (GeglTileMgr * self, 
                                      GeglImage* image, 
                                      GeglRect * area);
};


GType         gegl_tile_mgr_get_type            (void);
GeglTile *    gegl_tile_mgr_fetch_output_tile   (GeglTileMgr * self, 
                                                GeglOp * op, 
                                                GeglImage * dest, 
                                                GeglRect * area);
GeglTile *    gegl_tile_mgr_fetch_input_tile    (GeglTileMgr * self, 
                                                GeglImage * input, 
                                                GeglRect * area);
void          gegl_tile_mgr_release_tiles        (GeglTileMgr * self, 
                                                  GList * request_list);
GeglBuffer *  gegl_tile_mgr_create_buffer       (GeglTileMgr * self, 
                                                 GeglTile * tile);
GeglTile *    gegl_tile_mgr_get_tile            (GeglTileMgr * self, 
                                                GeglImage * image);
GeglTile *    gegl_tile_mgr_make_tile           (GeglTileMgr * self, 
                                                 GeglImage* image, 
                                                 GeglRect * area);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
