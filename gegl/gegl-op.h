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

   GList  *output_values;
};

typedef struct _GeglOpClass GeglOpClass;
struct _GeglOpClass 
{
   GeglNodeClass __parent__;

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

   void (* traverse)               (GeglOp * self);

   void (* compute_need_rects)     (GeglOp *self,
                                    GList *input_values);
   void (* compute_have_rect)      (GeglOp *self,
                                    GList * input_values); 
   void (* compute_derived_color_model)  (GeglOp *self,
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
void      gegl_op_compute_derived_color_model(GeglOp * self,
                                            GList *input_values);
void      gegl_op_evaluate                 (GeglOp * self, 
                                            GList * output_values,
                                            GList * input_values);
void      gegl_op_set_num_output_values    (GeglOp * self, 
                                            gint num_output_values);
GValue *  gegl_op_get_nth_output_value     (GeglOp *op, gint n); 
GValue *  gegl_op_get_nth_input_value      (GeglOp * op, gint n);
GList *   gegl_op_get_output_values        (GeglOp *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
