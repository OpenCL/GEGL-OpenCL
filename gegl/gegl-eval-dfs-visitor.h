#ifndef __GEGL_EVAL_DFS_VISITOR_H__
#define __GEGL_EVAL_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-dfs-visitor.h"

#define GEGL_TYPE_EVAL_DFS_VISITOR               (gegl_eval_dfs_visitor_get_type ())
#define GEGL_EVAL_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_DFS_VISITOR, GeglEvalDfsVisitor))
#define GEGL_EVAL_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_DFS_VISITOR, GeglEvalDfsVisitorClass))
#define GEGL_IS_EVAL_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_DFS_VISITOR))
#define GEGL_IS_EVAL_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_DFS_VISITOR))
#define GEGL_EVAL_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_DFS_VISITOR, GeglEvalDfsVisitorClass))

#ifndef __TYPEDEF_GEGL_EVAL_DFS_VISITOR__
#define __TYPEDEF_GEGL_EVAL_DFS_VISITOR__
typedef struct _GeglEvalDfsVisitor GeglEvalDfsVisitor;
#endif
struct _GeglEvalDfsVisitor 
{
    GeglDfsVisitor dfs_visitor;
};

typedef struct _GeglEvalDfsVisitorClass GeglEvalDfsVisitorClass;
struct _GeglEvalDfsVisitorClass 
{
    GeglDfsVisitorClass dfs_visitor_class;
};

GType           gegl_eval_dfs_visitor_get_type  (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
