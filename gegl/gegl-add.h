#ifndef __GEGL_ADD_H__
#define __GEGL_ADD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-unary.h"

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
   GeglUnary unary;

   /*< private >*/

   GValue *constants;
};

typedef struct _GeglAddClass GeglAddClass;
struct _GeglAddClass 
{
   GeglUnaryClass unary_class;
};

GType           gegl_add_get_type         (void);

void gegl_add_get_constants (GeglAdd * self, GValue *constants);
void gegl_add_set_constants (GeglAdd * self, GValue *constants);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
