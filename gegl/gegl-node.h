#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

typedef enum
{
   GEGL_NODE_VISIT_ENABLED = 1 << 0,
   GEGL_NODE_VISIT_DISABLED = 1 << 1,
   GEGL_NODE_VISIT_ALL = GEGL_NODE_VISIT_ENABLED | GEGL_NODE_VISIT_DISABLED,
   GEGL_NODE_VISIT_MASK = 0x03 
} GeglNodeVisitFlags;

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

typedef gboolean (*GeglNodeTraverseFunc)(GeglNode *node, gpointer data);

struct _GeglNode {
    GeglObject       __parent__;

    /*< private >*/
    gboolean enabled;
    GList   *inputs;
    GList   *outputs;
    gboolean visited;
    gboolean discovered;
    gint     shared_count;
    gint     num_inputs;
    gint     inheriting_input;
};

typedef struct _GeglNodeClass GeglNodeClass;
struct _GeglNodeClass {
   GeglObjectClass __parent__;
};

GType             gegl_node_get_type                  (void);
GeglNode *        gegl_node_set_nth_input             (GeglNode * self, 
                                                       GeglNode * input, 
                                                       gint n);
GeglNode *        gegl_node_get_nth_input             (GeglNode * self, 
                                                       gint n);
GeglNode *        gegl_node_get_nth_output            (GeglNode * self, 
                                                       guint n);
gint              gegl_node_input_multiplicity        (GeglNode * self, 
                                                       GeglNode * input);
gint              gegl_node_output_multiplicity       (GeglNode * self, 
                                                       GeglNode * output);
gboolean          gegl_node_is_leaf                   (GeglNode * self);
gboolean          gegl_node_is_root                   (GeglNode * self);
gint              gegl_node_num_inputs                (GeglNode * self);
gboolean          gegl_node_get_enabled               (GeglNode * self);
void              gegl_node_set_enabled               (GeglNode * self, 
                                                       gboolean enabled);
gint              gegl_node_num_outputs               (GeglNode * self);
gint              gegl_node_shared_count              (GeglNode * self);
void              gegl_node_traverse_depth_first      (GeglNode * self, 
                                                       GeglNodeTraverseFunc visit_func, 
                                                       gpointer data, 
                                                       gboolean init);
void              gegl_node_traverse_breadth_first    (GeglNode * self, 
                                                       GeglNodeTraverseFunc visit_func, 
                                                       gpointer data); 
GList *           gegl_node_get_outputs               (GeglNode * self);
GList *           gegl_node_get_inputs                (GeglNode * self);
void              gegl_node_set_inputs                (GeglNode * self, 
                                                       GList *list);
gint              gegl_node_inheriting_input          (GeglNode * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
