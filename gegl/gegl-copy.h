#ifndef __GEGL_COPY_H__
#define __GEGL_COPY_H__

/* File: gegl-copy.h
 * Author: Daniel S. Rogers
 * Date: 21 February, 2003
 *Derived from gegl-fade.h
 */
#include "gegl-unary.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COPY               (gegl_copy_get_type ())
#define GEGL_COPY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COPY, GeglCopy))
#define GEGL_COPY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COPY, GeglCopyClass))
#define GEGL_IS_COPY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COPY))
#define GEGL_IS_COPY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COPY))
#define GEGL_COPY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COPY, GeglCopyClass))

typedef struct _GeglCopy GeglCopy;
struct _GeglCopy 
{
   GeglUnary unary;

};

typedef struct _GeglCopyClass GeglCopyClass;
struct _GeglCopyClass 
{
   GeglUnaryClass unary_class;
};

GType            gegl_copy_get_type         (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
