#ifndef __GIL_BFS_VISITOR_H__
#define __GIL_BFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-visitor.h"

#define GIL_TYPE_BFS_VISITOR               (gil_bfs_visitor_get_type ())
#define GIL_BFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_BFS_VISITOR, GilBfsVisitor))
#define GIL_BFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_BFS_VISITOR, GilBfsVisitorClass))
#define GIL_IS_BFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_BFS_VISITOR))
#define GIL_IS_BFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_BFS_VISITOR))
#define GIL_BFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_BFS_VISITOR, GilBfsVisitorClass))

#ifndef __TYPEDEF_GIL_BFS_VISITOR__
#define __TYPEDEF_GIL_BFS_VISITOR__
typedef struct _GilBfsVisitor GilBfsVisitor;
#endif
struct _GilBfsVisitor
{
       GilVisitor __parent__;
};

typedef struct _GilBfsVisitorClass GilBfsVisitorClass;
struct _GilBfsVisitorClass
{
   GilVisitorClass __parent__;
};

GType         gil_bfs_visitor_get_type          (void);
void          gil_bfs_visitor_traverse          (GilBfsVisitor *self,
                                                  GilNode * node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
