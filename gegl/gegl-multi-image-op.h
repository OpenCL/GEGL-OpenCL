#ifndef __GEGL_MULTI_IMAGE_OP_H__
#define __GEGL_MULTI_IMAGE_OP_H__

#include "gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MULTI_IMAGE_OP               (gegl_multi_image_op_get_type ())
#define GEGL_MULTI_IMAGE_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MULTI_IMAGE_OP, GeglMultiImageOp))
#define GEGL_MULTI_IMAGE_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MULTI_IMAGE_OP, GeglMultiImageOpClass))
#define GEGL_IS_MULTI_IMAGE_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MULTI_IMAGE_OP))
#define GEGL_IS_MULTI_IMAGE_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MULTI_IMAGE_OP))
#define GEGL_MULTI_IMAGE_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MULTI_IMAGE_OP, GeglMultiImageOpClass))

typedef struct _GeglMultiImageOp GeglMultiImageOp;
struct _GeglMultiImageOp 
{
   GeglFilter filter;

   /*< private >*/
};

typedef struct _GeglMultiImageOpClass GeglMultiImageOpClass;
struct _GeglMultiImageOpClass 
{
   GeglFilterClass filter_class;

   void (* compute_need_rects)   (GeglMultiImageOp *self);
   void (* compute_have_rect)    (GeglMultiImageOp *self);
   void (* compute_color_model)  (GeglMultiImageOp *self);

};

GType           gegl_multi_image_op_get_type             (void);

void            gegl_multi_image_op_compute_need_rects (GeglMultiImageOp * self);
void            gegl_multi_image_op_compute_have_rect  (GeglMultiImageOp * self);
void            gegl_multi_image_op_compute_color_model(GeglMultiImageOp * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
