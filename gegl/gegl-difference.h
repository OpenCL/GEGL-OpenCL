#ifndef __GEGL_DIFFERENCE_H__
#define __GEGL_DIFFERENCE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-blend.h"

#define GEGL_TYPE_DIFFERENCE               (gegl_difference_get_type ())
#define GEGL_DIFFERENCE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DIFFERENCE, GeglDifference))
#define GEGL_DIFFERENCE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DIFFERENCE, GeglDifferenceClass))
#define GEGL_IS_DIFFERENCE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DIFFERENCE))
#define GEGL_IS_DIFFERENCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DIFFERENCE))
#define GEGL_DIFFERENCE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DIFFERENCE, GeglDifferenceClass))

#ifndef __TYPEDEF_GEGL_DIFFERENCE__
#define __TYPEDEF_GEGL_DIFFERENCE__
typedef struct _GeglDifference GeglDifference;
#endif
struct _GeglDifference 
{
   GeglBlend blend;
   /*< private >*/
};

typedef struct _GeglDifferenceClass GeglDifferenceClass;
struct _GeglDifferenceClass 
{
   GeglBlendClass blend_class;
};

GType            gegl_difference_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
