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
#ifndef __TYPEDEF_GEGL_INPUT_INFO__
#define __TYPEDEF_GEGL_INPUT_INFO__
typedef struct _GeglInputInfo  GeglInputInfo;
#endif

struct _GeglInputInfo 
{
  GeglNode * input;
  gint index;
};

struct _GeglNode 
{
    GeglObject       __parent__;

    /*< private >*/
    gint     num_inputs;
    GList   *input_infos;

    gint     num_outputs;
    GList   *outputs;

    gboolean enabled;
    gboolean visited;
    gboolean discovered;
    gint     shared_count;
};

typedef struct _GeglNodeClass GeglNodeClass;
struct _GeglNodeClass 
{
   GeglObjectClass __parent__;

   void (* accept)                  (GeglNode * self, 
                                     GeglVisitor * visitor);

};

GType             gegl_node_get_type                  (void);

void              gegl_node_set_nth_input_info        (GeglNode * self, 
                                                       GeglInputInfo * input_info,
                                                       gint n);

GeglInputInfo*    gegl_node_get_nth_input_info        (GeglNode * self, 
                                                       gint n);

GeglNode*         gegl_node_get_nth_input             (GeglNode * self, 
                                                       gint n);
void              gegl_node_set_nth_input             (GeglNode * self, 
                                                       GeglNode * input,
                                                       gint n);

gint              gegl_node_get_num_inputs            (GeglNode * self);

gint              gegl_node_get_num_outputs           (GeglNode * self);

GeglInputInfo*    gegl_node_get_input_infos           (GeglNode * self); 
GeglNode**        gegl_node_get_outputs               (GeglNode * self,
                                                       gint *index_num_outputs, 
                                                       gint index);

void              gegl_node_accept                    (GeglNode * self, 
                                                       GeglVisitor * visitor);

void              gegl_node_traverse_depth_first      (GeglNode * self, 
                                                       GeglVisitor *visitor, 
                                                       gboolean init);

void              gegl_node_traverse_breadth_first    (GeglNode * self, 
                                                       GeglVisitor *visitor);
/* protected */
void              gegl_node_set_num_inputs            (GeglNode * self,
                                                       gint num_inputs);
void              gegl_node_set_num_outputs           (GeglNode * self,
                                                       gint num_outputs);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
