#ifndef __GEGL_I_ADD_H__
#define __GEGL_I_ADD_H__

#include "gegl-binary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_I_ADD               (gegl_i_add_get_type ())
#define GEGL_I_ADD(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_I_ADD, GeglIAdd))
#define GEGL_I_ADD_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_I_ADD, GeglIAddClass))
#define GEGL_IS_I_ADD(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_I_ADD))
#define GEGL_IS_I_ADD_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_I_ADD))
#define GEGL_I_ADD_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_I_ADD, GeglIAddClass))

typedef struct _GeglIAdd GeglIAdd;
struct _GeglIAdd 
{
   GeglBinary binary;
   /*< private >*/
};

typedef struct _GeglIAddClass GeglIAddClass;
struct _GeglIAddClass 
{
   GeglBinaryClass binary_class;
};

GType            gegl_i_add_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
