#ifndef __GEGL_MOCK_BFS_VISITOR_H__
#define __GEGL_MOCK_BFS_VISITOR_H__

#include "gegl-bfs-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_BFS_VISITOR               (gegl_mock_bfs_visitor_get_type ())
#define GEGL_MOCK_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_BFS_VISITOR, GeglMockBfsVisitor))
#define GEGL_MOCK_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_BFS_VISITOR, GeglMockBfsVisitorClass))
#define GEGL_IS_MOCK_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_BFS_VISITOR))
#define GEGL_IS_MOCK_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_BFS_VISITOR))
#define GEGL_MOCK_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_BFS_VISITOR, GeglMockBfsVisitorClass))

typedef struct _GeglMockBfsVisitor GeglMockBfsVisitor;
struct _GeglMockBfsVisitor 
{
       GeglBfsVisitor bfs_visitor;
};

typedef struct _GeglMockBfsVisitorClass GeglMockBfsVisitorClass;
struct _GeglMockBfsVisitorClass 
{
       GeglBfsVisitorClass bfs_visitor;
};

GType         gegl_mock_bfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
