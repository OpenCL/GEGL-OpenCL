#ifndef __GEGL_CHECK_OP_H__
#define __GEGL_CHECK_OP_H__

#include "gegl-filter.h"
#include "gegl-image-op.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHECK_OP               (gegl_check_op_get_type ())
#define GEGL_CHECK_OP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHECK_OP, GeglCheckOp))
#define GEGL_CHECK_OP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHECK_OP, GeglCheckOpClass))
#define GEGL_IS_CHECK_OP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHECK_OP))
#define GEGL_IS_CHECK_OP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHECK_OP))
#define GEGL_CHECK_OP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHECK_OP, GeglCheckOpClass))


typedef struct _GeglCheckOp GeglCheckOp;
struct _GeglCheckOp 
{
    GeglFilter filter;

    /*< private >*/
    GeglImageOp *image_op;
    gboolean success;
};

typedef struct _GeglCheckOpClass GeglCheckOpClass;
struct _GeglCheckOpClass 
{
    GeglFilterClass filter_class;
};

GType           gegl_check_op_get_type           (void);

gboolean gegl_check_op_get_success(GeglCheckOp *self);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
