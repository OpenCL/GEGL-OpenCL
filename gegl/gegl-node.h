#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_VISITOR__
#define __TYPEDEF_GEGL_VISITOR__
typedef struct _GeglVisitor  GeglVisitor;
#endif

#define GEGL_TYPE_NODE               (gegl_node_get_type ())
#define GEGL_NODE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
#define GEGL_NODE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE, GeglNodeClass))
#define GEGL_IS_NODE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE))
#define GEGL_IS_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE))
#define GEGL_NODE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE, GeglNodeClass))

#ifndef __TYPEDEF_GEGL_NODE__
#define __TYPEDEF_GEGL_NODE__
typedef struct _GeglNode  GeglNode;
#endif

#ifndef __TYPEDEF_GEGL_CONNECTOR__
#define __TYPEDEF_GEGL_CONNECTOR__
typedef struct _GeglConnector  GeglConnector;
#endif
struct _GeglConnector
{
    GeglNode * node;
    gint input;

    GeglNode * source;
};

struct _GeglNode 
{
    GeglObject object;

    /*< private >*/
    gint     num_inputs;
    GeglConnector   *connectors;

    gint    num_outputs;
    GList  *sinks;

    gboolean enabled;
};

typedef struct _GeglNodeClass GeglNodeClass;
struct _GeglNodeClass 
{
    GeglObjectClass object_class;

    void (* accept)                  (GeglNode * self, 
                                      GeglVisitor * visitor);

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

void            gegl_node_accept                (GeglNode * self, 
                                                 GeglVisitor * visitor);

/* protected */
GList*          gegl_node_get_sinks             (GeglNode * self);
void            gegl_node_set_num_inputs        (GeglNode * self,
                                                 gint num_inputs);
void            gegl_node_set_num_outputs       (GeglNode * self,
                                                 gint num_outputs);
                                                                 
#ifdef __cplusplus
}
#endif /* __cplusplus */
                
#endif
