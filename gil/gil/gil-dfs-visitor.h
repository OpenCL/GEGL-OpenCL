#ifndef __GIL_DFS_VISITOR_H__
#define __GIL_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-visitor.h"

#define GIL_TYPE_DFS_VISITOR               (gil_dfs_visitor_get_type ())
#define GIL_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_DFS_VISITOR, GilDfsVisitor))
#define GIL_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_DFS_VISITOR, GilDfsVisitorClass))
#define GIL_IS_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_DFS_VISITOR))
#define GIL_IS_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_DFS_VISITOR))
#define GIL_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_DFS_VISITOR, GilDfsVisitorClass))

#ifndef __TYPEDEF_GIL_DFS_VISITOR__
#define __TYPEDEF_GIL_DFS_VISITOR__
typedef struct _GilDfsVisitor GilDfsVisitor;
#endif
struct _GilDfsVisitor
{
       GilVisitor __parent__;
};

typedef struct _GilDfsVisitorClass GilDfsVisitorClass;
struct _GilDfsVisitorClass
{
   GilVisitorClass __parent__;
};

GType         gil_dfs_visitor_get_type          (void);

void          gil_dfs_visitor_traverse(GilDfsVisitor * self,
                                       GilNode * node);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
