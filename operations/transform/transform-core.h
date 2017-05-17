#ifndef __OP_TRANSFORM_H__
#define __OP_TRANSFORM_H__

#include <gegl-matrix.h>

G_BEGIN_DECLS

#define TYPE_OP_TRANSFORM               (op_transform_get_type ())
#define OP_TRANSFORM(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_OP_TRANSFORM, OpTransform))
#define OP_TRANSFORM_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_OP_TRANSFORM, OpTransformClass))
#define IS_OP_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_OP_TRANSFORM))
#define IS_OP_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_OP_TRANSFORM))
#define OP_TRANSFORM_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_OP_TRANSFORM, OpTransformClass))

typedef struct _OpTransform OpTransform;

struct _OpTransform
{
  GeglOperationFilter parent_instance;
  gdouble             origin_x;
  gdouble             origin_y;
  GeglSamplerType     sampler;
  gboolean            clip_to_input;
};

typedef struct _OpTransformClass OpTransformClass;

struct _OpTransformClass
{
  GeglOperationFilterClass parent_class;

  void (* create_matrix) (OpTransform  *transform,
                          GeglMatrix3  *matrix);
};

GType op_transform_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
