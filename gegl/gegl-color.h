#ifndef __GEGL_COLOR_H__
#define __GEGL_COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-no-input.h"

#define GEGL_TYPE_COLOR               (gegl_color_get_type ())
#define GEGL_COLOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR, GeglColor))
#define GEGL_COLOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR, GeglColorClass))
#define GEGL_IS_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR))
#define GEGL_IS_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR))
#define GEGL_COLOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR, GeglColorClass))

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor GeglColor;
#endif
struct _GeglColor 
{
   GeglNoInput no_input;

   /*< private >*/
   GValue * pixel;
   gint  width;
   gint  height;
};

typedef struct _GeglColorClass GeglColorClass;
struct _GeglColorClass 
{
   GeglNoInputClass no_input_class;
};

GType           gegl_color_get_type         (void);

void            gegl_color_get_pixel        (GeglColor * self, 
                                             GValue *pixel);
void            gegl_color_set_pixel        (GeglColor * self, 
                                             GValue *pixel);
gint            gegl_color_get_width    (GeglColor * self);
gint            gegl_color_get_height   (GeglColor * self);
void            gegl_color_set_width    (GeglColor * self, gint width);
void            gegl_color_set_height   (GeglColor * self, gint height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
