#ifndef __GEGL_DATA_SPACE_H__
#define __GEGL_DATA_SPACE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#define GEGL_TYPE_DATA_SPACE               (gegl_data_space_get_type ())
#define GEGL_DATA_SPACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DATA_SPACE, GeglDataSpace))
#define GEGL_DATA_SPACE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DATA_SPACE, GeglDataSpaceClass))
#define GEGL_IS_DATA_SPACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DATA_SPACE))
#define GEGL_IS_DATA_SPACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DATA_SPACE))
#define GEGL_DATA_SPACE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DATA_SPACE, GeglDataSpaceClass))


#ifndef __TYPEDEF_GEGL_DATA_SPACE__
#define __TYPEDEF_GEGL_DATA_SPACE__
typedef struct _GeglDataSpace  GeglDataSpace;
#endif
struct _GeglDataSpace 
{
   GeglObject object;

   GeglDataSpaceType data_space_type;
   gboolean is_channel_data;
   gint bits;
   gchar *name;
};


typedef struct _GeglDataSpaceClass GeglDataSpaceClass;
struct _GeglDataSpaceClass 
{
  GeglObjectClass object_class;

  /* If the data space represents channel data */
  void  (*convert_to_float) (GeglDataSpace * self, 
                             gfloat * dest, 
                             gpointer src, 
                             gint num);

  void  (*convert_from_float) (GeglDataSpace * self, 
                               gpointer dest, 
                               gfloat * src, 
                               gint num);

  void  (*convert_value_to_float) (GeglDataSpace * self, 
                                   GValue * dest,        /* float data */ 
                                   GValue * src);
  void  (*convert_value_from_float) (GeglDataSpace * self, 
                                     GValue * dest, 
                                     GValue * src);      /* float data */
};


GType           gegl_data_space_get_type       (void);

gboolean        gegl_data_space_register         (GeglDataSpace *data_space);
GeglDataSpace * gegl_data_space_instance         (gchar *data_space_name);

GeglDataSpaceType gegl_data_space_data_space_type (GeglDataSpace * self);
gchar *         gegl_data_space_name             (GeglDataSpace * self);
gint            gegl_data_space_bits             (GeglDataSpace * self);
gboolean        gegl_data_space_is_channel_data  (GeglDataSpace * self);

void            gegl_data_space_convert_to_float (GeglDataSpace * self, 
                                                  gfloat * dest, 
                                                  gpointer src, 
                                                  gint num);
void            gegl_data_space_convert_from_float (GeglDataSpace * self, 
                                                    gpointer dest, 
                                                    gfloat * src, 
                                                    gint num);

void            gegl_data_space_convert_value_to_float (GeglDataSpace * self, 
                                                        GValue * dest, 
                                                        GValue * src);

void            gegl_data_space_convert_value_from_float (GeglDataSpace * self, 
                                                          GValue * dest, 
                                                          GValue * src);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
