#ifndef __GEGL_BFS_VISITOR_H__
#define __GEGL_BFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-visitor.h"

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif

#define GEGL_TYPE_BFS_VISITOR               (gegl_bfs_visitor_get_type ())
#define GEGL_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BFS_VISITOR, GeglBfsVisitor))
#define GEGL_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BFS_VISITOR, GeglBfsVisitorClass))
#define GEGL_IS_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BFS_VISITOR))
#define GEGL_IS_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BFS_VISITOR))
#define GEGL_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BFS_VISITOR, GeglBfsVisitorClass))

#ifndef __TYPEDEF_GEGL_BFS_VISITOR__
#define __TYPEDEF_GEGL_BFS_VISITOR__
typedef struct _GeglBfsVisitor GeglBfsVisitor;
#endif
struct _GeglBfsVisitor {
       GeglVisitor __parent__;
       
       GList * input_values;
       GeglFilter * filter;
};

typedef struct _GeglBfsVisitorClass GeglBfsVisitorClass;
struct _GeglBfsVisitorClass {
       GeglVisitorClass __parent__;
};

GType         gegl_bfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
