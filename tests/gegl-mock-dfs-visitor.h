#ifndef __GEGL_MOCK_DFS_VISITOR_H__
#define __GEGL_MOCK_DFS_VISITOR_H__

#include "gegl-dfs-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_DFS_VISITOR               (gegl_mock_dfs_visitor_get_type ())
#define GEGL_MOCK_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_DFS_VISITOR, GeglMockDfsVisitor))
#define GEGL_MOCK_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_DFS_VISITOR, GeglMockDfsVisitorClass))
#define GEGL_IS_MOCK_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_DFS_VISITOR))
#define GEGL_IS_MOCK_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_DFS_VISITOR))
#define GEGL_MOCK_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_DFS_VISITOR, GeglMockDfsVisitorClass))

typedef struct _GeglMockDfsVisitor GeglMockDfsVisitor;
struct _GeglMockDfsVisitor 
{
       GeglDfsVisitor dfs_visitor;
};

typedef struct _GeglMockDfsVisitorClass GeglMockDfsVisitorClass;
struct _GeglMockDfsVisitorClass 
{
       GeglDfsVisitorClass dfs_visitor;
};

GType         gegl_mock_dfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
