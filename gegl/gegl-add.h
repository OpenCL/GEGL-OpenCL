#ifndef __GEGL_ADD_H__
#define __GEGL_ADD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_ADD               (gegl_add_get_type ())
#define GEGL_ADD(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_ADD, GeglAdd))
#define GEGL_ADD_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_ADD, GeglAddClass))
#define GEGL_IS_ADD(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_ADD))
#define GEGL_IS_ADD_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_ADD))
#define GEGL_ADD_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_ADD, GeglAddClass))

#ifndef __TYPEDEF_GEGL_ADD__
#define __TYPEDEF_GEGL_ADD__
typedef struct _GeglAdd GeglAdd;
#endif
struct _GeglAdd 
{
   GeglPointOp point_op;
   /*< private >*/
};

typedef struct _GeglAddClass GeglAddClass;
struct _GeglAddClass 
{
   GeglPointOpClass point_op_class;
};

GType            gegl_add_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
