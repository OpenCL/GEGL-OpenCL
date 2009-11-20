#ifndef __OP_AFFINE_H__
#define __OP_AFFINE_H__

#include "gegl-buffer-private.h"
#include <gegl-matrix.h>

G_BEGIN_DECLS

#define TYPE_OP_AFFINE               (op_affine_get_type ())
#define OP_AFFINE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_OP_AFFINE, OpAffine))
#define OP_AFFINE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_OP_AFFINE, OpAffineClass))
#define IS_OP_AFFINE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_OP_AFFINE))
#define IS_OP_AFFINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_OP_AFFINE))
#define OP_AFFINE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_OP_AFFINE, OpAffineClass))

typedef void (*OpAffineCreateMatrixFunc) (GeglOperation *op,
                                          GeglMatrix3        matrix);
typedef struct _OpAffine OpAffine;
struct _OpAffine
{
  GeglOperationFilter parent;

  GeglMatrix3  matrix;
  gdouble      origin_x,
               origin_y;
  gchar       *filter;
  gboolean     hard_edges;
  gint         lanczos_width;
};

typedef struct _OpAffineClass OpAffineClass;
struct _OpAffineClass
{
  GeglOperationFilterClass parent_class;

  OpAffineCreateMatrixFunc create_matrix;
};

GType op_affine_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
