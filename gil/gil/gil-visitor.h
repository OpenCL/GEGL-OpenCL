#ifndef __GIL_VISITOR_H__
#define __GIL_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib-object.h>

#ifndef __TYPEDEF_GIL_NODE__
#define __TYPEDEF_GIL_NODE__
typedef struct _GilNode GilNode;
#endif

#define GIL_TYPE_VISITOR               (gil_visitor_get_type ())
#define GIL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_VISITOR, GilVisitor))
#define GIL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_VISITOR, GilVisitorClass))
#define GIL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_VISITOR))
#define GIL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_VISITOR))
#define GIL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_VISITOR, GilVisitorClass))

#ifndef __TYPEDEF_GIL_NODE_INFO__
#define __TYPEDEF_GIL_NODE_INFO__
typedef struct _GilNodeInfo GilNodeInfo;
#endif
struct _GilNodeInfo 
{
  gboolean visited;
  gboolean discovered;
  gint shared_count;
}; 

#ifndef __TYPEDEF_GIL_VISITOR__
#define __TYPEDEF_GIL_VISITOR__
typedef struct _GilVisitor GilVisitor;
#endif
struct _GilVisitor 
{
       GObject __parent__;

       GList * visits_list;
       GList * visits_objects_list;
       GHashTable *nodes_hash;
};

typedef struct _GilVisitorClass GilVisitorClass;
struct _GilVisitorClass 
{
   GObjectClass __parent__;

   void (* visit_node)   (GilVisitor *self,
                          GilNode * node);
};

GType         gil_visitor_get_type          (void); 

void          gil_visitor_visit_node        (GilVisitor *self,
                                             GilNode * node);
GList *       gil_visitor_get_visits_list   (GilVisitor *self);
GList *       gil_visitor_get_visits_objects_list   (GilVisitor *self);

void          gil_visitor_node_insert       (GilVisitor *self, 
                                              GilNode *node);
GilNodeInfo* gil_visitor_node_lookup       (GilVisitor *self, 
                                              GilNode *node);

gboolean      gil_visitor_get_visited       (GilVisitor *self, 
                                              GilNode *node);
void          gil_visitor_set_visited       (GilVisitor *self,
                                              GilNode *node,
                                              gboolean visited);
gboolean      gil_visitor_get_discovered    (GilVisitor *self,
                                              GilNode *node);
void          gil_visitor_set_discovered    (GilVisitor *self,
                                              GilNode *node,
                                              gboolean discovered);
gint          gil_visitor_get_shared_count  (GilVisitor *self,
                                              GilNode *node);
void          gil_visitor_set_shared_count  (GilVisitor *self, 
                                              GilNode *node, 
                                              gint shared_count);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
