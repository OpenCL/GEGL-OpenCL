#ifndef __GEGL_VISITOR_H__
#define __GEGL_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_NODE__
#define __TYPEDEF_GEGL_NODE__
typedef struct _GeglNode GeglNode;
#endif

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif

#define GEGL_TYPE_VISITOR               (gegl_visitor_get_type ())
#define GEGL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITOR, GeglVisitor))
#define GEGL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VISITOR, GeglVisitorClass))
#define GEGL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITOR))
#define GEGL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VISITOR))
#define GEGL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VISITOR, GeglVisitorClass))

#ifndef __TYPEDEF_GEGL_VISITOR__
#define __TYPEDEF_GEGL_VISITOR__
typedef struct _GeglVisitor GeglVisitor;
#endif
struct _GeglVisitor {
       GeglObject __parent__;

       GList * visits_list;
};

typedef struct _GeglVisitorClass GeglVisitorClass;
struct _GeglVisitorClass {
   GeglObjectClass __parent__;

   void (* visit_node)              (GeglVisitor *self,
                                     GeglNode * node);
   void (* visit_op)                (GeglVisitor *self,
                                     GeglOp * op);
   void (* visit_filter)            (GeglVisitor *self,
                                     GeglFilter * filter);
};

GType         gegl_visitor_get_type          (void); 

void          gegl_visitor_visit_node        (GeglVisitor *self,
                                              GeglNode * node);
void          gegl_visitor_visit_op          (GeglVisitor *self,
                                              GeglOp * op);
void          gegl_visitor_visit_filter      (GeglVisitor *self,
                                              GeglFilter * filter);
GList *       gegl_visitor_get_visits_list   (GeglVisitor *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
