#ifndef __GEGL_MOCK_NODE_H__
#define __GEGL_MOCK_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../gegl/gegl-node.h"

#define GEGL_TYPE_MOCK_NODE               (gegl_mock_node_get_type ())
#define GEGL_MOCK_NODE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_NODE, GeglMockNode))
#define GEGL_MOCK_NODE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_NODE, GeglMockNodeClass))
#define GEGL_IS_MOCK_NODE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_NODE))
#define GEGL_IS_MOCK_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_NODE))
#define GEGL_MOCK_NODE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_NODE, GeglMockNodeClass))

#ifndef __TYPEDEF_GEGL_MOCK_NODE__
#define __TYPEDEF_GEGL_MOCK_NODE__
typedef struct _GeglMockNode  GeglMockNode;
#endif

struct _GeglMockNode 
{
    GeglNode       node;

    /*< private >*/
};

typedef struct _GeglMockNodeClass GeglMockNodeClass;
struct _GeglMockNodeClass 
{
   GeglNodeClass node_class;
};

GType             gegl_mock_node_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
