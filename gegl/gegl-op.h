#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-node.h"

#ifndef __TYPEDEF_GEGL_SAMPLED_IMAGE__
#define __TYPEDEF_GEGL_SAMPLED_IMAGE__
typedef struct _GeglSampledImage  GeglSampledImage;
#endif

#define GEGL_TYPE_OP               (gegl_op_get_type ())
#define GEGL_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OP, GeglOp))
#define GEGL_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OP, GeglOpClass))
#define GEGL_IS_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OP))
#define GEGL_IS_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OP))
#define GEGL_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OP, GeglOpClass))

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp GeglOp;
#endif
struct _GeglOp {
   GeglNode __parent__;

   /*< private >*/
   gint alt_input;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass {
   GeglNodeClass __parent__;

   void (* prepare)                (GeglOp * self, 
                                    GList * request_list);
   void (* process)                (GeglOp * self, 
                                    GList * request_list);
   void (* finish)                 (GeglOp * self, 
                                    GList * request_list);
};

void      gegl_op_prepare              (GeglOp * self, 
                                        GList * request_list);
void      gegl_op_process              (GeglOp * self, 
                                        GList * request_list);
void      gegl_op_finish               (GeglOp * self, 
                                        GList * request_list);

GType            gegl_op_get_type         (void);
gint             gegl_op_get_alt_input    (GeglOp * self);
void             gegl_op_apply            (GeglOp * self, 
                                           GeglSampledImage * dest, 
                                           GeglRect * roi);

GeglOp*          gegl_op_get_source0      (GeglOp *self);
void             gegl_op_set_source0      (GeglOp *op, 
                                           GeglOp *source0);
GeglOp*          gegl_op_get_source1      (GeglOp *self);
void             gegl_op_set_source1      (GeglOp *op, 
                                           GeglOp *source1);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
