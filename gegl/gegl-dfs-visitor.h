#ifndef __GEGL_DFS_VISITOR_H__
#define __GEGL_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-visitor.h"

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif

#define GEGL_TYPE_DFS_VISITOR               (gegl_dfs_visitor_get_type ())
#define GEGL_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DFS_VISITOR, GeglDfsVisitor))
#define GEGL_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DFS_VISITOR, GeglDfsVisitorClass))
#define GEGL_IS_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DFS_VISITOR))
#define GEGL_IS_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DFS_VISITOR))
#define GEGL_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DFS_VISITOR, GeglDfsVisitorClass))

#ifndef __TYPEDEF_GEGL_DFS_VISITOR__
#define __TYPEDEF_GEGL_DFS_VISITOR__
typedef struct _GeglDfsVisitor GeglDfsVisitor;
#endif
struct _GeglDfsVisitor {
       GeglVisitor __parent__;
       
       GList * input_values;
       GeglFilter * filter;
};

typedef struct _GeglDfsVisitorClass GeglDfsVisitorClass;
struct _GeglDfsVisitorClass {
       GeglVisitorClass __parent__;
};

GType         gegl_dfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
