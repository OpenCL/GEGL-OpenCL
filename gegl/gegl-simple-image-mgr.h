#ifndef __GEGL_SIMPLE_IMAGE_MGR_H__
#define __GEGL_SIMPLE_IMAGE_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-image-mgr.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE__
#define __TYPEDEF_GEGL_IMAGE__
typedef struct _GeglImage GeglImage;
#endif

#define GEGL_TYPE_SIMPLE_IMAGE_MGR               (gegl_simple_image_mgr_get_type ())
#define GEGL_SIMPLE_IMAGE_MGR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SIMPLE_IMAGE_MGR, GeglSimpleImageMgr))
#define GEGL_SIMPLE_IMAGE_MGR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SIMPLE_IMAGE_MGR, GeglSimpleImageMgrClass))
#define GEGL_IS_SIMPLE_IMAGE_MGR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SIMPLE_IMAGE_MGR))
#define GEGL_IS_SIMPLE_IMAGE_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SIMPLE_IMAGE_MGR))
#define GEGL_SIMPLE_IMAGE_MGR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SIMPLE_IMAGE_MGR, GeglSimpleImageMgrClass))

#ifndef __TYPEDEF_GEGL_SIMPLE_IMAGE_MGR__
#define __TYPEDEF_GEGL_SIMPLE_IMAGE_MGR__
typedef struct _GeglSimpleImageMgr GeglSimpleImageMgr;
#endif
struct _GeglSimpleImageMgr {
   GeglImageMgr __parent__;

   /*< private >*/
   GeglRect roi;
};

typedef struct _GeglSimpleImageMgrClass GeglSimpleImageMgrClass;
struct _GeglSimpleImageMgrClass {
   GeglImageMgrClass __parent__;
};


GType         gegl_simple_image_mgr_get_type            (void);

GeglTile *    gegl_simple_image_mgr_get_tile            (GeglSimpleImageMgr * self, 
                                                         GeglImage * image);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
