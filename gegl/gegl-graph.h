#ifndef __GEGL_GRAPH_H__
#define __GEGL_GRAPH_H__
                                                
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op.h"

#define GEGL_TYPE_GRAPH               (gegl_graph_get_type ())
#define GEGL_GRAPH(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_GRAPH, GeglGraph))
#define GEGL_GRAPH_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_GRAPH, GeglGraphClass))
#define GEGL_IS_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_GRAPH))
#define GEGL_IS_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_GRAPH))
#define GEGL_GRAPH_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_GRAPH, GeglGraphClass))

#ifndef __TYPEDEF_GEGL_GRAPH__
#define __TYPEDEF_GEGL_GRAPH__
typedef struct _GeglGraph GeglGraph;
#endif

#ifndef __TYPEDEF_GEGL_GRAPH_INPUT__
#define __TYPEDEF_GEGL_GRAPH_INPUT_
typedef struct _GeglGraphInput GeglGraphInput;
#endif
struct _GeglGraphInput
{
  GeglNode * node;
  gint node_input;

  GeglGraph * graph;
  gint graph_input;
};

#ifndef __TYPEDEF_GEGL_GRAPH_OUTPUT__
#define __TYPEDEF_GEGL_GRAPH_OUTPUT_
typedef struct _GeglGraphOutput GeglGraphOutput;
#endif
struct _GeglGraphOutput
{
  GeglNode * node;
  gint node_output;

  GeglGraph * graph;
  gint graph_output;
};

struct _GeglGraph 
{
   GeglOp op;

   /*< private >*/
   GeglNode * root;

   GList * graph_inputs;
   GList * graph_outputs;
};

typedef struct _GeglGraphClass GeglGraphClass;
struct _GeglGraphClass 
{
   GeglOpClass op_class;
};

GType           gegl_graph_get_type             (void);
GeglNode *      gegl_graph_get_root             (GeglGraph * self); 
void            gegl_graph_set_root             (GeglGraph * self, 
                                                 GeglNode *root);
GeglGraphInput *gegl_graph_lookup_input         (GeglGraph *self,
                                                 GeglNode *node,
                                                 gint node_input);
GeglGraphOutput*gegl_graph_lookup_output        (GeglGraph *self,
                                                 gint graph_output);
GeglNode *      gegl_graph_find_source          (GeglGraph *self,
                                                 gint * output,
                                                 GeglNode *node,
                                                 gint index);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
