#ifndef __GEGL_FILTER_DFS_VISITOR_H__
#define __GEGL_FILTER_DFS_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-visitor.h"

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif

#define GEGL_TYPE_FILTER_DFS_VISITOR               (gegl_filter_dfs_visitor_get_type ())
#define GEGL_FILTER_DFS_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILTER_DFS_VISITOR, GeglFilterDfsVisitor))
#define GEGL_FILTER_DFS_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILTER_DFS_VISITOR, GeglFilterDfsVisitorClass))
#define GEGL_IS_FILTER_DFS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILTER_DFS_VISITOR))
#define GEGL_IS_FILTER_DFS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILTER_DFS_VISITOR))
#define GEGL_FILTER_DFS_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILTER_DFS_VISITOR, GeglFilterDfsVisitorClass))

#ifndef __TYPEDEF_GEGL_FILTER_DFS_VISITOR__
#define __TYPEDEF_GEGL_FILTER_DFS_VISITOR__
typedef struct _GeglFilterDfsVisitor GeglFilterDfsVisitor;
#endif
struct _GeglFilterDfsVisitor {
       GeglVisitor __parent__;
       
       GList * input_values;
       GeglFilter * filter;
};

typedef struct _GeglFilterDfsVisitorClass GeglFilterDfsVisitorClass;
struct _GeglFilterDfsVisitorClass {
       GeglVisitorClass __parent__;
};

GType         gegl_filter_dfs_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
