#ifndef __GEGL_COLOR_H__
#define __GEGL_COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

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
    GeglObject object;

    /*< private >*/
    GeglColorModel * color_model;
    GeglChannelValue * channel_values;
    gint num_channels;
};

typedef struct _GeglColorClass GeglColorClass;
struct _GeglColorClass 
{
    GeglObjectClass object_class;
};

GType           gegl_color_get_type              (void);
GeglChannelValue*gegl_color_get_channel_values   (GeglColor * self);
GeglColorModel *gegl_color_get_color_model       (GeglColor * self);
int             gegl_color_get_num_channels      (GeglColor * self);
void            gegl_color_set_channel_values    (GeglColor * self,
                                                  GeglChannelValue * channel_values);
void            gegl_color_set                   (GeglColor * self,
                                                  GeglColor * color);
void            gegl_color_set_constant          (GeglColor * self,
                                                  GeglColorConstant constant);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
