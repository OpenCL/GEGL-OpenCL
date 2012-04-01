#ifndef __OP_AFFINE_H__
#define __OP_AFFINE_H__

#include "gegl-buffer-private.h"
#include <gegl-matrix.h>

G_BEGIN_DECLS

#define TYPE_OP_AFFINE               (op_affine_get_type ())
#define OP_AFFINE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_OP_AFFINE, OpTransform))
#define OP_AFFINE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_OP_AFFINE, OpTransformClass))
#define IS_OP_AFFINE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_OP_AFFINE))
#define IS_OP_AFFINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_OP_AFFINE))
#define OP_AFFINE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_OP_AFFINE, OpTransformClass))

typedef struct _OpTransform OpTransform;

struct _OpTransform
{
  GeglOperationFilter parent_instance;
  gdouble             origin_x;
  gdouble             origin_y;
  gchar              *filter;
  gboolean            hard_edges;
  gint                lanczos_width;
};

typedef struct _OpTransformClass OpTransformClass;

struct _OpTransformClass
{
  GeglOperationFilterClass parent_class;

  void (* create_matrix) (OpTransform    *affine,
                          GeglMatrix3  *matrix);
};

GType op_affine_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
