#ifndef __GEGL_COMP_PREMULT_H__
#define __GEGL_COMP_PREMULT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-point-op.h"

#define GEGL_TYPE_COMP_PREMULT               (gegl_comp_premult_get_type ())
#define GEGL_COMP_PREMULT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMP_PREMULT, GeglCompPremult))
#define GEGL_COMP_PREMULT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMP_PREMULT, GeglCompPremultClass))
#define GEGL_IS_COMP_PREMULT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMP_PREMULT))
#define GEGL_IS_COMP_PREMULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMP_PREMULT))
#define GEGL_COMP_PREMULT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMP_PREMULT, GeglCompPremultClass))

#ifndef __TYPEDEF_GEGL_COMP_PREMULT__
#define __TYPEDEF_GEGL_COMP_PREMULT__
typedef struct _GeglCompPremult GeglCompPremult;
#endif
struct _GeglCompPremult 
{
   GeglPointOp point_op;

   /*< private >*/
   GeglCompositeMode comp_mode; 
};

typedef struct _GeglCompPremultClass GeglCompPremultClass;
struct _GeglCompPremultClass 
{
   GeglPointOpClass point_op_class;
};

GType            gegl_comp_premult_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
