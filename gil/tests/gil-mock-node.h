#ifndef __GIL_MOCK_NODE_H__
#define __GIL_MOCK_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../gil/gil-node.h"

#define GIL_TYPE_MOCK_NODE               (gil_mock_node_get_type ())
#define GIL_MOCK_NODE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_MOCK_NODE, GilMockNode))
#define GIL_MOCK_NODE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_MOCK_NODE, GilMockNodeClass))
#define GIL_IS_MOCK_NODE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_MOCK_NODE))
#define GIL_IS_MOCK_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_MOCK_NODE))
#define GIL_MOCK_NODE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_MOCK_NODE, GilMockNodeClass))

#ifndef __TYPEDEF_GIL_MOCK_NODE__
#define __TYPEDEF_GIL_MOCK_NODE__
typedef struct _GilMockNode  GilMockNode;
#endif

struct _GilMockNode
{
    GilNode       __parent__;

    /*< private >*/
};

typedef struct _GilMockNodeClass GilMockNodeClass;
struct _GilMockNodeClass
{
   GilNodeClass __parent__;
};

GType             gil_mock_node_get_type                  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
