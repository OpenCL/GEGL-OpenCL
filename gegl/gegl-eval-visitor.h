#ifndef __GEGL_EVAL_VISITOR_H__
#define __GEGL_EVAL_VISITOR_H__

#include "gegl-dfs-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_EVAL_VISITOR               (gegl_eval_visitor_get_type ())
#define GEGL_EVAL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitor))
#define GEGL_EVAL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitorClass))
#define GEGL_IS_EVAL_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_VISITOR))
#define GEGL_IS_EVAL_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_VISITOR))
#define GEGL_EVAL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitorClass))

typedef struct _GeglEvalVisitor GeglEvalVisitor;
struct _GeglEvalVisitor 
{
    GeglDfsVisitor dfs_visitor;
};

typedef struct _GeglEvalVisitorClass GeglEvalVisitorClass;
struct _GeglEvalVisitorClass 
{
    GeglDfsVisitorClass dfs_visitor_class;
};

GType           gegl_eval_visitor_get_type      (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
