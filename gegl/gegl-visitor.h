#ifndef __GEGL_VISITOR_H__
#define __GEGL_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_NODE__
#define __TYPEDEF_GEGL_NODE__
typedef struct _GeglNode GeglNode;
#endif

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif

#ifndef __TYPEDEF_GEGL_GRAPH__
#define __TYPEDEF_GEGL_GRAPH__
typedef struct _GeglGraph GeglGraph;
#endif

#define GEGL_TYPE_VISITOR               (gegl_visitor_get_type ())
#define GEGL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITOR, GeglVisitor))
#define GEGL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VISITOR, GeglVisitorClass))
#define GEGL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITOR))
#define GEGL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VISITOR))
#define GEGL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VISITOR, GeglVisitorClass))

#ifndef __TYPEDEF_GEGL_NODE_INFO__
#define __TYPEDEF_GEGL_NODE_INFO__
typedef struct _GeglNodeInfo GeglNodeInfo;
#endif
struct _GeglNodeInfo 
{
  gboolean visited;
  gboolean discovered;
  gint shared_count;
}; 

#ifndef __TYPEDEF_GEGL_VISITOR__
#define __TYPEDEF_GEGL_VISITOR__
typedef struct _GeglVisitor GeglVisitor;
#endif
struct _GeglVisitor 
{
       GeglObject object;

       GList * visits_list;
       GHashTable *nodes_hash;
       GeglGraph * graph;
};

typedef struct _GeglVisitorClass GeglVisitorClass;
struct _GeglVisitorClass 
{
   GeglObjectClass object_class;

   void (* visit_node)             (GeglVisitor *self,
                                    GeglNode * node);
   void (* visit_op)               (GeglVisitor *self,
                                    GeglOp * op);
   void (* visit_filter)           (GeglVisitor *self,
                                    GeglFilter * filter);
   void (* visit_graph)            (GeglVisitor *self,
                                    GeglGraph * graph);
};

GType           gegl_visitor_get_type           (void); 

void            gegl_visitor_visit_node         (GeglVisitor *self,
                                                 GeglNode * node);
void            gegl_visitor_visit_filter       (GeglVisitor *self,
                                                 GeglFilter * filter);
void            gegl_visitor_visit_op           (GeglVisitor *self,
                                                 GeglOp * op);
void            gegl_visitor_visit_graph        (GeglVisitor *self,
                                                 GeglGraph * graph);

GList *         gegl_visitor_get_visits_list    (GeglVisitor *self);

void            gegl_visitor_node_insert        (GeglVisitor *self, 
                                                 GeglNode *node);
GeglNodeInfo*   gegl_visitor_node_lookup        (GeglVisitor *self, 
                                                 GeglNode *node);
gboolean        gegl_visitor_get_visited        (GeglVisitor *self, 
                                                 GeglNode *node);
void            gegl_visitor_set_visited        (GeglVisitor *self,
                                                 GeglNode *node,
                                                 gboolean visited);
gboolean        gegl_visitor_get_discovered     (GeglVisitor *self,
                                                 GeglNode *node);
void            gegl_visitor_set_discovered     (GeglVisitor *self,
                                                 GeglNode *node,
                                                 gboolean discovered);
gint            gegl_visitor_get_shared_count   (GeglVisitor *self,
                                                 GeglNode *node);
void            gegl_visitor_set_shared_count   (GeglVisitor *self, 
                                                 GeglNode *node, 
                                                 gint shared_count);
GList *        gegl_visitor_collect_data_list(GeglVisitor *self,
                                          GeglNode *node);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
