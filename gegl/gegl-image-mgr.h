#ifndef __GEGL_IMAGE_MGR_H__
#define __GEGL_IMAGE_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_SAMPLED_IMAGE__
#define __TYPEDEF_GEGL_SAMPLED_IMAGE__
typedef struct _GeglSampledImage  GeglSampledImage;
#endif

#ifndef __TYPEDEF_GEGL_TILE_MGR__
#define __TYPEDEF_GEGL_TILE_MGR__
typedef struct _GeglTileMgr  GeglTileMgr;
#endif

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp  GeglOp;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE__
#define __TYPEDEF_GEGL_IMAGE__
typedef struct _GeglImage  GeglImage;
#endif

#define GEGL_TYPE_IMAGE_MGR               (gegl_image_mgr_get_type ())
#define GEGL_IMAGE_MGR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_MGR, GeglImageMgr))
#define GEGL_IMAGE_MGR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_MGR, GeglImageMgrClass))
#define GEGL_IS_IMAGE_MGR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_MGR))
#define GEGL_IS_IMAGE_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_MGR))
#define GEGL_IMAGE_MGR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_MGR, GeglImageMgrClass))

#ifndef __TYPEDEF_GEGL_IMAGE_MGR__
#define __TYPEDEF_GEGL_IMAGE_MGR__
typedef struct _GeglImageMgr GeglImageMgr;
#endif
struct _GeglImageMgr {
   GeglObject __parent__;

   /*< private >*/
   GeglTileMgr * tile_mgr;
};

typedef struct _GeglImageMgrClass GeglImageMgrClass;
struct _GeglImageMgrClass {
   GeglObjectClass __parent__;

   void         (* apply)                           (GeglImageMgr * self, 
                                                     GeglOp * root, 
                                                     GeglSampledImage * dest, 
                                                     GeglRect * roi);
   void         (* delete_image)                    (GeglImageMgr * self, 
                                                     GeglImage * image);
   void         (* add_image)                       (GeglImageMgr * self, 
                                                     GeglImage* image);
};

void               gegl_image_mgr_install           (GeglImageMgr *image_mgr);
GeglImageMgr * gegl_image_mgr_instance          (void);
GType              gegl_image_mgr_get_type          (void);

void               gegl_image_mgr_apply             (GeglImageMgr * self, 
                                                         GeglOp * root, 
                                                         GeglSampledImage * dest, 
                                                         GeglRect * roi);
void               gegl_image_mgr_delete_image      (GeglImageMgr * self, 
                                                         GeglImage * image);
void               gegl_image_mgr_add_image         (GeglImageMgr * self, 
                                                         GeglImage * image);
GeglTileMgr *      gegl_image_mgr_get_tile_mgr      (GeglImageMgr * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
