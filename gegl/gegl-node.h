#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

#include "gegl-object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _GeglVisitor;

#define GEGL_TYPE_NODE               (gegl_node_get_type ())
#define GEGL_NODE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
#define GEGL_NODE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE, GeglNodeClass))
#define GEGL_IS_NODE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE))
#define GEGL_IS_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE))
#define GEGL_NODE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE, GeglNodeClass))

typedef struct _GeglNode  GeglNode;
struct _GeglNode 
{
    GeglObject object;

    /*< private >*/
    GArray *inputs;
    GArray *outputs;

    gboolean enabled;
};

typedef struct _GeglNodeClass GeglNodeClass;
struct _GeglNodeClass 
{
    GeglObjectClass object_class;

    void (* accept)                  (GeglNode * self, 
                                      struct _GeglVisitor * visitor);

};

GType           gegl_node_get_type              (void);
                                                
gint            gegl_node_get_num_inputs        (GeglNode * self);
gint            gegl_node_get_num_outputs       (GeglNode * self);

GeglNode *      gegl_node_get_source            (GeglNode * self, 
                                                 gint n);
void            gegl_node_set_source            (GeglNode * self, 
                                                 GeglNode * source,
                                                 gint n);
gint            gegl_node_get_num_sinks         (GeglNode * self);
GeglNode*       gegl_node_get_sink              (GeglNode * self,
                                                 gint n);
gint            gegl_node_get_sink_input        (GeglNode * self,
                                                 gint n);
void            gegl_node_unlink                (GeglNode * self);
void            gegl_node_remove_sources        (GeglNode *self); 
void            gegl_node_remove_sinks          (GeglNode *self); 

/* protected */
void            gegl_node_set_num_inputs        (GeglNode * self,
                                                 gint num_inputs);
void            gegl_node_set_num_outputs       (GeglNode * self,
                                                 gint num_outputs);
void            gegl_node_add_input             (GeglNode *self);
void            gegl_node_add_output            (GeglNode *self);
void            gegl_node_accept                (GeglNode * self, 
                                                 struct _GeglVisitor * visitor);
                                                                 
#ifdef __cplusplus
}
#endif /* __cplusplus */
                
#endif
