#ifndef __GEGL_GRAPH_BFS_VISITOR_H__
#define __GEGL_GRAPH_BFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-bfs-visitor.h"

#ifndef __TYPEDEF_GEGL_GRAPH__
#define __TYPEDEF_GEGL_GRAPH__
typedef struct _GeglGraph GeglGraph;
#endif

#define GEGL_TYPE_GRAPH_BFS_VISITOR               (gegl_graph_bfs_visitor_get_type ())
#define GEGL_GRAPH_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_GRAPH_BFS_VISITOR, GeglGraphBfsVisitor))
#define GEGL_GRAPH_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_GRAPH_BFS_VISITOR, GeglGraphBfsVisitorClass))
#define GEGL_IS_GRAPH_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_GRAPH_BFS_VISITOR))
#define GEGL_IS_GRAPH_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_GRAPH_BFS_VISITOR))
#define GEGL_GRAPH_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_GRAPH_BFS_VISITOR, GeglGraphBfsVisitorClass))

#ifndef __TYPEDEF_GEGL_GRAPH_BFS_VISITOR__
#define __TYPEDEF_GEGL_GRAPH_BFS_VISITOR__
typedef struct _GeglGraphBfsVisitor GeglGraphBfsVisitor;
#endif
struct _GeglGraphBfsVisitor 
{
       GeglBfsVisitor bfs_visitor;
       
       GeglGraph * graph;
};

typedef struct _GeglGraphBfsVisitorClass GeglGraphBfsVisitorClass;
struct _GeglGraphBfsVisitorClass 
{
       GeglBfsVisitorClass bfs_visitor_class;
};

GType         gegl_graph_bfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
