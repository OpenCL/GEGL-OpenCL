#ifndef __GEGL_IMAGE_OP_H__
#define __GEGL_IMAGE_OP_H__

#include "gegl-filter.h"

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
};

typedef struct _GeglImageOpClass GeglImageOpClass;
struct _GeglImageOpClass 
{
   GeglFilterClass filter_class;
};

GType           gegl_image_op_get_type             (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
