#ifndef __GIL_NODE_H__
#define __GIL_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib-object.h>
#include "gil-types.h"

#ifndef __TYPEDEF_GIL_VISITOR__
#define __TYPEDEF_GIL_VISITOR__
typedef struct _GilVisitor  GilVisitor;
#endif

#define GIL_TYPE_NODE               (gil_node_get_type ())
#define GIL_NODE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_NODE, GilNode))
#define GIL_NODE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_NODE, GilNodeClass))
#define GIL_IS_NODE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_NODE))
#define GIL_IS_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_NODE))
#define GIL_NODE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_NODE, GilNodeClass))

#ifndef __TYPEDEF_GIL_NODE__
#define __TYPEDEF_GIL_NODE__
typedef struct _GilNode  GilNode;
#endif

struct _GilNode 
{
    GObject     __parent__;

    /*< private >*/
    gchar * name;
    GList  *children;
};

typedef struct _GilNodeClass GilNodeClass;
struct _GilNodeClass 
{
   GObjectClass __parent__;

   void (* accept)        (GilNode * self, 
                           GilVisitor * visitor);

};

GType             gil_node_get_type                  (void);
GilNode*          gil_node_get_nth_child             (GilNode * self, 
                                                      gint n);
void              gil_node_set_nth_child             (GilNode * self, 
                                                      GilNode * child,
                                                      gint n);
gint              gil_node_get_num_children          (GilNode * self);
void              gil_node_set_num_children          (GilNode * self,
                                                      gint num_children);

void              gil_node_accept                    (GilNode * self, 
                                                      GilVisitor * visitor);

void              gil_node_set_name                  (GilNode * self, 
                                                      const gchar * name);
G_CONST_RETURN gchar* 
                  gil_node_get_name                  (GilNode * self);
void              gil_node_append_child              (GilNode *self, 
                                                      GilNode *child);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
