#ifndef __GIL_MOCK_VISITOR_H__
#define __GIL_MOCK_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gil-visitor.h"

#define GIL_TYPE_MOCK_VISITOR               (gil_mock_visitor_get_type ())
#define GIL_MOCK_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIL_TYPE_MOCK_VISITOR, GilMockVisitor))
#define GIL_MOCK_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GIL_TYPE_MOCK_VISITOR, GilMockVisitorClass))
#define GIL_IS_MOCK_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIL_TYPE_MOCK_VISITOR))
#define GIL_IS_MOCK_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIL_TYPE_MOCK_VISITOR))
#define GIL_MOCK_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIL_TYPE_MOCK_VISITOR, GilMockVisitorClass))

#ifndef __TYPEDEF_GIL_MOCK_VISITOR__
#define __TYPEDEF_GIL_MOCK_VISITOR__
typedef struct _GilMockVisitor GilMockVisitor;
#endif
struct _GilMockVisitor {
       GilVisitor __parent__;
};

typedef struct _GilMockVisitorClass GilMockVisitorClass;
struct _GilMockVisitorClass {
       GilVisitorClass __parent__;
};

GType         gil_mock_visitor_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
