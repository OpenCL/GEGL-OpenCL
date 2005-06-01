#ifndef __GEGL_FILL_H__
#define __GEGL_FILL_H__

#include "gegl-no-input.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_FILL               (gegl_fill_get_type ())
#define GEGL_FILL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILL, GeglFill))
#define GEGL_FILL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILL, GeglFillClass))
#define GEGL_IS_FILL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILL))
#define GEGL_IS_FILL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILL))
#define GEGL_FILL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILL, GeglFillClass))

typedef struct _GeglFill GeglFill;
struct _GeglFill
{
   GeglNoInput no_input;

   /*< private >*/
};

typedef struct _GeglFillClass GeglFillClass;
struct _GeglFillClass
{
   GeglNoInputClass no_input_class;
};

GType           gegl_fill_get_type         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
