#ifndef __GEGL_GRAPH_DFS_VISITOR_H__
#define __GEGL_GRAPH_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-dfs-visitor.h"

#ifndef __TYPEDEF_GEGL_GRAPH__
#define __TYPEDEF_GEGL_GRAPH__
typedef struct _GeglGraph GeglGraph;
#endif

#define GEGL_TYPE_GRAPH_DFS_VISITOR               (gegl_graph_dfs_visitor_get_type ())
#define GEGL_GRAPH_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_GRAPH_DFS_VISITOR, GeglGraphDfsVisitor))
#define GEGL_GRAPH_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_GRAPH_DFS_VISITOR, GeglGraphDfsVisitorClass))
#define GEGL_IS_GRAPH_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_GRAPH_DFS_VISITOR))
#define GEGL_IS_GRAPH_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_GRAPH_DFS_VISITOR))
#define GEGL_GRAPH_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_GRAPH_DFS_VISITOR, GeglGraphDfsVisitorClass))

#ifndef __TYPEDEF_GEGL_GRAPH_DFS_VISITOR__
#define __TYPEDEF_GEGL_GRAPH_DFS_VISITOR__
typedef struct _GeglGraphDfsVisitor GeglGraphDfsVisitor;
#endif
struct _GeglGraphDfsVisitor 
{
       GeglDfsVisitor dfs_visitor;
       
       GeglGraph * graph;
};

typedef struct _GeglGraphDfsVisitorClass GeglGraphDfsVisitorClass;
struct _GeglGraphDfsVisitorClass 
{
       GeglDfsVisitorClass dfs_visitor_class;
};

GType         gegl_graph_dfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
