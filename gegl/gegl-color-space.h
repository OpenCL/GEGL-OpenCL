#ifndef __GEGL_COLOR_SPACE_H__
#define __GEGL_COLOR_SPACE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#define GEGL_TYPE_COLOR_SPACE               (gegl_color_space_get_type ())
#define GEGL_COLOR_SPACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_SPACE, GeglColorSpace))
#define GEGL_COLOR_SPACE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_SPACE, GeglColorSpaceClass))
#define GEGL_IS_COLOR_SPACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_SPACE))
#define GEGL_IS_COLOR_SPACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_SPACE))
#define GEGL_COLOR_SPACE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_SPACE, GeglColorSpaceClass))

#ifndef __TYPEDEF_GEGL_COLOR_SPACE__
#define __TYPEDEF_GEGL_COLOR_SPACE__
typedef struct _GeglColorSpace  GeglColorSpace;
#endif
struct _GeglColorSpace 
{
   GeglObject object;

   GeglColorSpaceType color_space_type;
   gint num_components;
   gboolean is_additive;
   gboolean is_subtractive;
   gchar *name;
   gchar **component_names;
};


typedef struct _GeglColorSpaceClass GeglColorSpaceClass;
struct _GeglColorSpaceClass 
{
  GeglObjectClass object_class;

  void  (*convert_to_xyz) (GeglColorSpace * self, 
                           gfloat * dest, 
                           gfloat * src, 
                           gint num);

  void  (*convert_from_xyz) (GeglColorSpace * self, 
                             gfloat * dest, 
                             gfloat * src, 
                             gint num);
};


GType           gegl_color_space_get_type       (void);

gboolean        gegl_color_space_register       (GeglColorSpace *color_space);
GeglColorSpace *gegl_color_space_instance       (gchar *name);
GeglColorSpaceType 
                gegl_color_space_color_space_type (GeglColorSpace * self);
gint            gegl_color_space_num_components (GeglColorSpace * self);
gchar *         gegl_color_space_name           (GeglColorSpace * self);
gchar *         gegl_color_space_component_name (GeglColorSpace * self, 
                                                 gint n);

void            gegl_color_space_convert_to_xyz (GeglColorSpace * self, 
                                                 gfloat * dest, 
                                                 gfloat * src, 
                                                 gint num);
void            gegl_color_space_convert_from_xyz (GeglColorSpace * self, 
                                                   gfloat * dest, 
                                                   gfloat * src, 
                                                   gint num);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
