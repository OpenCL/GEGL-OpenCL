#ifndef __GEGL_EVAL_MGR_H__
#define __GEGL_EVAL_MGR_H__

#include "gegl-object.h"
#include "gegl-node.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_EVAL_MGR               (gegl_eval_mgr_get_type ())
#define GEGL_EVAL_MGR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_MGR, GeglEvalMgr))
#define GEGL_EVAL_MGR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))
#define GEGL_IS_EVAL_MGR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_MGR))
#define GEGL_IS_EVAL_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_MGR))
#define GEGL_EVAL_MGR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))

typedef struct _GeglEvalMgr GeglEvalMgr;
struct _GeglEvalMgr 
{
   GeglObject object;

   /*< private >*/
   GeglNode * root;
   GeglRect roi;
};

typedef struct _GeglEvalMgrClass GeglEvalMgrClass;
struct _GeglEvalMgrClass 
{
   GeglObjectClass object_class;

   void (*evaluate)                 (GeglEvalMgr * self);
};

GType           gegl_eval_mgr_get_type          (void);
GeglNode *      gegl_eval_mgr_get_root          (GeglEvalMgr * self); 
void            gegl_eval_mgr_set_root          (GeglEvalMgr * self, 
                                                 GeglNode *root);
void            gegl_eval_mgr_get_roi           (GeglEvalMgr *self, 
                                                 GeglRect *roi);
void            gegl_eval_mgr_set_roi           (GeglEvalMgr *self, 
                                                 GeglRect *roi);

void            gegl_eval_mgr_evaluate          (GeglEvalMgr * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
