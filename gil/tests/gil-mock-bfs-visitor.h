#ifndef __GIL_MOCK_BFS_VISITOR_H__
#define __GIL_MOCK_BFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-bfs-visitor.h"

#define GIL_TYPE_MOCK_BFS_VISITOR               (gil_mock_bfs_visitor_get_type ())
#define GIL_MOCK_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_MOCK_BFS_VISITOR, GilMockBfsVisitor))
#define GIL_MOCK_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_MOCK_BFS_VISITOR, GilMockBfsVisitorClass))
#define GIL_IS_MOCK_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_MOCK_BFS_VISITOR))
#define GIL_IS_MOCK_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_MOCK_BFS_VISITOR))
#define GIL_MOCK_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_MOCK_BFS_VISITOR, GilMockBfsVisitorClass))

#ifndef __TYPEDEF_GIL_MOCK_BFS_VISITOR__
#define __TYPEDEF_GIL_MOCK_BFS_VISITOR__
typedef struct _GilMockBfsVisitor GilMockBfsVisitor;
#endif
struct _GilMockBfsVisitor {
       GilBfsVisitor __parent__;
};

typedef struct _GilMockBfsVisitorClass GilMockBfsVisitorClass;
struct _GilMockBfsVisitorClass {
       GilBfsVisitorClass __parent__;
};

GType         gil_mock_bfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
