#ifndef __GEGL_DUMP_VISITOR_H__
#define __GEGL_DUMP_VISITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-visitor.h"

#define GEGL_TYPE_DUMP_VISITOR               (gegl_dump_visitor_get_type ())
#define GEGL_DUMP_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitor))
#define GEGL_DUMP_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitorClass))
#define GEGL_IS_DUMP_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DUMP_VISITOR))
#define GEGL_IS_DUMP_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DUMP_VISITOR))
#define GEGL_DUMP_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitorClass))

#ifndef __TYPEDEF_GEGL_DUMP_VISITOR__
#define __TYPEDEF_GEGL_DUMP_VISITOR__
typedef struct _GeglDumpVisitor GeglDumpVisitor;
#endif
struct _GeglDumpVisitor 
{
       GeglVisitor visitor;
       gint tabs;
       GeglGraph * graph;
};

typedef struct _GeglDumpVisitorClass GeglDumpVisitorClass;
struct _GeglDumpVisitorClass 
{
       GeglVisitorClass visitor_class;
};

GType         gegl_dump_visitor_get_type          (void); 
void          gegl_dump_visitor_traverse          (GeglDumpVisitor * self, 
                                                   GeglNode * node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
