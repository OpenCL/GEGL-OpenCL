#ifndef __GEGL_ADD_H__
#define __GEGL_ADD_H__

#include "gegl-unary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_ADD               (gegl_add_get_type ())
#define GEGL_ADD(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_ADD, GeglAdd))
#define GEGL_ADD_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_ADD, GeglAddClass))
#define GEGL_IS_ADD(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_ADD))
#define GEGL_IS_ADD_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_ADD))
#define GEGL_ADD_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_ADD, GeglAddClass))

typedef struct _GeglAdd GeglAdd;
typedef struct _GeglAddClass GeglAddClass;

struct _GeglAdd
{
   GeglUnary unary;

   /*< private >*/
};

struct _GeglAddClass
{
   GeglUnaryClass unary_class;
};

GType           gegl_add_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
