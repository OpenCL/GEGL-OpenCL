#ifndef __GEGL_MOCK_VISITOR_H__
#define __GEGL_MOCK_VISITOR_H__

#include "gegl-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_VISITOR               (gegl_mock_visitor_get_type ())
#define GEGL_MOCK_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_VISITOR, GeglMockVisitor))
#define GEGL_MOCK_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_VISITOR, GeglMockVisitorClass))
#define GEGL_IS_MOCK_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_VISITOR))
#define GEGL_IS_MOCK_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_VISITOR))
#define GEGL_MOCK_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_VISITOR, GeglMockVisitorClass))

typedef struct _GeglMockVisitor GeglMockVisitor;
struct _GeglMockVisitor 
{
       GeglVisitor visitor;
};

typedef struct _GeglMockVisitorClass GeglMockVisitorClass;
struct _GeglMockVisitorClass 
{
       GeglVisitorClass visitor_class;
};

GType         gegl_mock_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
