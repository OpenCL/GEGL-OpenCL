#ifndef __GEGL_EVAL_BFS_VISITOR_H__
#define __GEGL_EVAL_BFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-bfs-visitor.h"

#define GEGL_TYPE_EVAL_BFS_VISITOR               (gegl_eval_bfs_visitor_get_type ())
#define GEGL_EVAL_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_BFS_VISITOR, GeglEvalBfsVisitor))
#define GEGL_EVAL_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_BFS_VISITOR, GeglEvalBfsVisitorClass))
#define GEGL_IS_EVAL_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_BFS_VISITOR))
#define GEGL_IS_EVAL_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_BFS_VISITOR))
#define GEGL_EVAL_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_BFS_VISITOR, GeglEvalBfsVisitorClass))

#ifndef __TYPEDEF_GEGL_EVAL_BFS_VISITOR__
#define __TYPEDEF_GEGL_EVAL_BFS_VISITOR__
typedef struct _GeglEvalBfsVisitor GeglEvalBfsVisitor;
#endif
struct _GeglEvalBfsVisitor 
{
    GeglBfsVisitor bfs_visitor;
};

typedef struct _GeglEvalBfsVisitorClass GeglEvalBfsVisitorClass;
struct _GeglEvalBfsVisitorClass 
{
    GeglBfsVisitorClass bfs_visitor_class;
};

GType           gegl_eval_bfs_visitor_get_type  (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
