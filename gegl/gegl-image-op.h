#ifndef __GEGL_IMAGE_OP_H__
#define __GEGL_IMAGE_OP_H__

#include "gegl-filter.h"
#include "gegl-color-model.h"
#include "gegl-image.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_IMAGE_OP               (gegl_image_op_get_type ())
#define GEGL_IMAGE_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_OP, GeglImageOp))
#define GEGL_IMAGE_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE_OP, GeglImageOpClass))
#define GEGL_IS_IMAGE_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_OP))
#define GEGL_IS_IMAGE_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE_OP))
#define GEGL_IMAGE_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE_OP, GeglImageOpClass))

typedef struct _GeglImageOp GeglImageOp;
struct _GeglImageOp 
{
   GeglFilter filter;

   /*< private >*/
   GeglImage * image;
};

typedef struct _GeglImageOpClass GeglImageOpClass;
struct _GeglImageOpClass 
{
   GeglFilterClass filter_class;

   void (* compute_need_rect)       (GeglImageOp *self,
                                     GeglRect *input_need_rect,
                                     GeglRect *need_rect,
                                     gint i);
   void (* compute_have_rect)       (GeglImageOp *self,
                                     GeglRect *have_rect,
                                     GList * data_inputs); 
   GeglColorModel* 
        (* compute_color_model)     (GeglImageOp *self,
                                     GList * data_inputs); 

};

GType           gegl_image_op_get_type             (void);

void            gegl_image_op_set_image (GeglImageOp * self, 
                                         GeglImage *image);
GeglImage *     gegl_image_op_get_image (GeglImageOp * self);


void            gegl_image_op_compute_need_rect   (GeglImageOp * self,
                                                   GeglRect * input_need_rect,
                                                   GeglRect * need_rect,
                                                   gint i);
void            gegl_image_op_compute_have_rect   (GeglImageOp * self,
                                                   GeglRect *have_rect,
                                                   GList * data_inputs);
GeglColorModel* gegl_image_op_compute_color_model (GeglImageOp * self, 
                                                   GList * data_inputs);

void            gegl_image_op_evaluate_need_rects   (GeglImageOp *self, 
                                                     GList *data_inputs);
void            gegl_image_op_evaluate_have_rect    (GeglImageOp * self,
                                                     GList * data_inputs);
void            gegl_image_op_evaluate_color_model  (GeglImageOp * self, 
                                                     GList * data_inputs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
