#ifndef __GEGL_PRINT_IMPL_H__
#define __GEGL_PRINT_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-stat-op-impl.h"

#define GEGL_TYPE_PRINT_IMPL               (gegl_print_impl_get_type ())
#define GEGL_PRINT_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PRINT_IMPL, GeglPrintImpl))
#define GEGL_PRINT_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PRINT_IMPL, GeglPrintImplClass))
#define GEGL_IS_PRINT_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PRINT_IMPL))
#define GEGL_IS_PRINT_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PRINT_IMPL))
#define GEGL_PRINT_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PRINT_IMPL, GeglPrintImplClass))

#ifndef __TYPEDEF_GEGL_PRINT_IMPL__
#define __TYPEDEF_GEGL_PRINT_IMPL__
typedef struct _GeglPrintImpl GeglPrintImpl;
#endif
struct _GeglPrintImpl {
   GeglStatOpImpl __parent__;

   /*< private >*/
   gchar * buffer;
   gint buffer_size;
   gchar * current;
   gint left;
   gboolean use_log;
};

typedef struct _GeglPrintImplClass GeglPrintImplClass;
struct _GeglPrintImplClass {
   GeglStatOpImplClass __parent__;
};

GType           gegl_print_impl_get_type              (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
