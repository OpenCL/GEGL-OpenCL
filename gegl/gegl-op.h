#ifndef __GEGL_OP_H__
#define __GEGL_OP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-node.h"

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
struct _GeglOp 
{
   GeglNode __parent__;

   /*< private >*/

   GValue *output_value;
   GParamSpec *output_param_spec;

   GList *input_param_specs;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass 
{
   GeglNodeClass __parent__;

   void (* apply)                  (GeglOp * self);

   void (* evaluate)               (GeglOp * self, 
                                    GList * output_values,
                                    GList * input_values);
   void (* prepare)                (GeglOp * self, 
                                    GList * output_values,
                                    GList * input_values);
   void (* process)                (GeglOp * self, 
                                    GList * output_values,
                                    GList * input_values);
   void (* finish)                 (GeglOp * self, 
                                    GList * output_values,
                                    GList * input_values);

   void (* compute_need_rect)      (GeglOp *self,
                                    GValue *input_value,
                                    gint i); 
   void (* compute_have_rect)      (GeglOp *self,
                                    GList * input_values); 
};

GType     gegl_op_get_type                 (void);

void      gegl_op_apply                    (GeglOp * self);
void      gegl_op_apply_image              (GeglOp * self,
                                            GeglOp * image,
                                            GeglRect *roi);
void      gegl_op_apply_roi                (GeglOp * self, 
                                            GeglRect *roi);

void      gegl_op_compute_need_rects       (GeglOp * self,
                                            GList *input_values);
void      gegl_op_compute_have_rect        (GeglOp * self,
                                            GList *input_values);
void      gegl_op_compute_need_rect        (GeglOp * self,
                                            GValue * input_value,
                                            gint i);

void      gegl_op_evaluate                 (GeglOp * self, 
                                            GList * output_values,
                                            GList * input_values);
GValue *  gegl_op_get_output_value         (GeglOp *op); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
