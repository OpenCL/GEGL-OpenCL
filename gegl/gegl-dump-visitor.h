#ifndef __GEGL_DUMP_VISITOR_H__
#define __GEGL_DUMP_VISITOR_H__

#include "gegl-visitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_DUMP_VISITOR               (gegl_dump_visitor_get_type ())
#define GEGL_DUMP_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitor))
#define GEGL_DUMP_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitorClass))
#define GEGL_IS_DUMP_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DUMP_VISITOR))
#define GEGL_IS_DUMP_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DUMP_VISITOR))
#define GEGL_DUMP_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DUMP_VISITOR, GeglDumpVisitorClass))

typedef struct _GeglDumpVisitor GeglDumpVisitor;
struct _GeglDumpVisitor 
{
    GeglVisitor visitor;
    gint tabs;
};

typedef struct _GeglDumpVisitorClass GeglDumpVisitorClass;
struct _GeglDumpVisitorClass 
{
    GeglVisitorClass visitor_class;
};

GType           gegl_dump_visitor_get_type      (void); 
void            gegl_dump_visitor_traverse      (GeglDumpVisitor * self, 
                                                 GeglNode * node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
