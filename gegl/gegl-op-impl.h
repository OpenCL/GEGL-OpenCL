#ifndef __GEGL_OP_IMPL_H__
#define __GEGL_OP_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"


#define GEGL_TYPE_OP_IMPL               (gegl_op_impl_get_type ())
#define GEGL_OP_IMPL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OP_IMPL, GeglOpImpl))
#define GEGL_OP_IMPL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OP_IMPL, GeglOpImplClass))
#define GEGL_IS_OP_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OP_IMPL))
#define GEGL_IS_OP_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OP_IMPL))
#define GEGL_OP_IMPL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OP_IMPL, GeglOpImplClass))

#ifndef __TYPEDEF_GEGL_OP_IMPL__
#define __TYPEDEF_GEGL_OP_IMPL__
typedef struct _GeglOpImpl GeglOpImpl;
#endif
struct _GeglOpImpl {
   GeglObject __parent__;

   /*< private >*/
   gint num_inputs;
};

typedef struct _GeglOpImplClass GeglOpImplClass;
struct _GeglOpImplClass {
   GeglObjectClass __parent__;

   void (* prepare)                (GeglOpImpl * self, 
                                    GList * request_list);
   void (* process)                (GeglOpImpl * self, 
                                    GList * request_list);
   void (* finish)                 (GeglOpImpl * self, 
                                    GList * request_list);
};

GType     gegl_op_impl_get_type             (void);

void      gegl_op_impl_prepare              (GeglOpImpl * self, 
                                            GList * request_list);
void      gegl_op_impl_process              (GeglOpImpl * self, 
                                            GList * request_list);
void      gegl_op_impl_finish               (GeglOpImpl * self, 
                                            GList * request_list);

gint      gegl_op_impl_get_num_inputs       (GeglOpImpl * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
