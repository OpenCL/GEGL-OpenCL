#ifndef __GEGL_MOCK_NODE_VISITOR_H__
#define __GEGL_MOCK_NODE_VISITOR_H__

#include "gegl-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_NODE_VISITOR               (gegl_mock_node_visitor_get_type ())
#define GEGL_MOCK_NODE_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_NODE_VISITOR, GeglMockNodeVisitor))
#define GEGL_MOCK_NODE_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_NODE_VISITOR, GeglMockNodeVisitorClass))
#define GEGL_IS_MOCK_NODE_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_NODE_VISITOR))
#define GEGL_IS_MOCK_NODE_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_NODE_VISITOR))
#define GEGL_MOCK_NODE_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_NODE_VISITOR, GeglMockNodeVisitorClass))

typedef struct _GeglMockNodeVisitor GeglMockNodeVisitor;
struct _GeglMockNodeVisitor 
{
       GeglVisitor visitor;
};

typedef struct _GeglMockNodeVisitorClass GeglMockNodeVisitorClass;
struct _GeglMockNodeVisitorClass 
{
       GeglVisitorClass visitor_class;
};

GType         gegl_mock_node_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
