#ifndef __GIL_MOCK_DFS_VISITOR_H__
#define __GIL_MOCK_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-dfs-visitor.h"

#define GIL_TYPE_MOCK_DFS_VISITOR               (gil_mock_dfs_visitor_get_type ())
#define GIL_MOCK_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_MOCK_DFS_VISITOR, GilMockDfsVisitor))
#define GIL_MOCK_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_MOCK_DFS_VISITOR, GilMockDfsVisitorClass))
#define GIL_IS_MOCK_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_MOCK_DFS_VISITOR))
#define GIL_IS_MOCK_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_MOCK_DFS_VISITOR))
#define GIL_MOCK_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_MOCK_DFS_VISITOR, GilMockDfsVisitorClass))

#ifndef __TYPEDEF_GIL_MOCK_DFS_VISITOR__
#define __TYPEDEF_GIL_MOCK_DFS_VISITOR__
typedef struct _GilMockDfsVisitor GilMockDfsVisitor;
#endif
struct _GilMockDfsVisitor {
       GilDfsVisitor __parent__;
};

typedef struct _GilMockDfsVisitorClass GilMockDfsVisitorClass;
struct _GilMockDfsVisitorClass {
       GilDfsVisitorClass __parent__;
};

GType         gil_mock_dfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
