#ifndef __GEGL_DFS_VISITOR_H__
#define __GEGL_DFS_VISITOR_H__

#include "gegl-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_DFS_VISITOR               (gegl_dfs_visitor_get_type ())
#define GEGL_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DFS_VISITOR, GeglDfsVisitor))
#define GEGL_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DFS_VISITOR, GeglDfsVisitorClass))
#define GEGL_IS_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DFS_VISITOR))
#define GEGL_IS_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DFS_VISITOR))
#define GEGL_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DFS_VISITOR, GeglDfsVisitorClass))

typedef struct _GeglDfsVisitor GeglDfsVisitor;
struct _GeglDfsVisitor 
{
    GeglVisitor visitor;
};

typedef struct _GeglDfsVisitorClass GeglDfsVisitorClass;
struct _GeglDfsVisitorClass 
{
    GeglVisitorClass visitor_class;
};

GType           gegl_dfs_visitor_get_type       (void); 
void            gegl_dfs_visitor_traverse       (GeglDfsVisitor * self, 
                                                 GeglNode * node); 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
