#ifndef __GEGL_COPY_H__
#define __GEGL_COPY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"
#include "gegl-color-model.h"
      
#define GEGL_TYPE_COPY               (gegl_copy_get_type ())
#define GEGL_COPY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COPY, GeglCopy))
#define GEGL_COPY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COPY, GeglCopyClass))
#define GEGL_IS_COPY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COPY))
#define GEGL_IS_COPY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COPY))
#define GEGL_COPY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COPY, GeglCopyClass))

#ifndef __TYPEDEF_GEGL_COPY__
#define __TYPEDEF_GEGL_COPY__
typedef struct _GeglCopy GeglCopy;
#endif
struct _GeglCopy 
{
   GeglPointOp point_op;

   /*< private >*/
   gfloat * float_xyz_data[4];
   GeglConvertFunc convert_func;
};

typedef struct _GeglCopyClass GeglCopyClass;
struct _GeglCopyClass 
{
   GeglPointOpClass point_op_class;
};

GType                gegl_copy_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
