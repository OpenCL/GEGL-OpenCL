#ifndef __GEGL_XOR_H__
#define __GEGL_XOR_H__

#include "gegl-comp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_XOR               (gegl_xor_get_type ())
#define GEGL_XOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_XOR, GeglXor))
#define GEGL_XOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_XOR, GeglXorClass))
#define GEGL_IS_XOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_XOR))
#define GEGL_IS_XOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_XOR))
#define GEGL_XOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_XOR, GeglXorClass))

typedef struct _GeglXor GeglXor;
struct _GeglXor 
{
   GeglComp comp;

   /*< private >*/
};

typedef struct _GeglXorClass GeglXorClass;
struct _GeglXorClass 
{
   GeglCompClass comp_class;
};

GType           gegl_xor_get_type         (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
