#ifndef __GEGL_PRINT_H__
#define __GEGL_PRINT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-stat-op.h"

#define GEGL_TYPE_PRINT               (gegl_print_get_type ())
#define GEGL_PRINT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PRINT, GeglPrint))
#define GEGL_PRINT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PRINT, GeglPrintClass))
#define GEGL_IS_PRINT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PRINT))
#define GEGL_IS_PRINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PRINT))
#define GEGL_PRINT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PRINT, GeglPrintClass))

#ifndef __TYPEDEF_GEGL_PRINT__
#define __TYPEDEF_GEGL_PRINT__
typedef struct _GeglPrint GeglPrint;
#endif
struct _GeglPrint {
   GeglStatOp __parent__;

   /*< private >*/
   gchar * buffer;
   gint buffer_size;
   gchar * current;
   gint left;
   gboolean use_log;
};

typedef struct _GeglPrintClass GeglPrintClass;
struct _GeglPrintClass {
   GeglStatOpClass __parent__;
};

GType       gegl_print_get_type       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
