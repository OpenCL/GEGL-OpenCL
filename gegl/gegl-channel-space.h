#ifndef __GEGL_CHANNEL_SPACE_H__
#define __GEGL_CHANNEL_SPACE_H__

#include "gegl-object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHANNEL_SPACE               (gegl_channel_space_get_type ())
#define GEGL_CHANNEL_SPACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNEL_SPACE, GeglChannelSpace))
#define GEGL_CHANNEL_SPACE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNEL_SPACE, GeglChannelSpaceClass))
#define GEGL_IS_CHANNEL_SPACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNEL_SPACE))
#define GEGL_IS_CHANNEL_SPACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNEL_SPACE))
#define GEGL_CHANNEL_SPACE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNEL_SPACE, GeglChannelSpaceClass))


typedef struct _GeglChannelSpace  GeglChannelSpace;
struct _GeglChannelSpace 
{
   GeglObject object;

   /*< private >*/
   GeglChannelSpaceType channel_space_type;
   GType channel_type;
   gboolean is_channel_data;
   gint bits;
   gchar *name;
};


typedef struct _GeglChannelSpaceClass GeglChannelSpaceClass;
struct _GeglChannelSpaceClass 
{
  GeglObjectClass object_class;

  /* If the data space represents channel data */
  void  (*convert_to_float) (GeglChannelSpace * self, 
                             gfloat * dest, 
                             gpointer src, 
                             gint num);

  void  (*convert_from_float) (GeglChannelSpace * self, 
                               gpointer dest, 
                               gfloat * src, 
                               gint num);

  void  (*convert_value_to_float) (GeglChannelSpace * self, 
                                   GValue * dest,        /* float data */ 
                                   GValue * src);
  void  (*convert_value_from_float) (GeglChannelSpace * self, 
                                     GValue * dest, 
                                     GValue * src);      /* float data */
};


GType           gegl_channel_space_get_type           (void);

gboolean        gegl_channel_space_register           (GeglChannelSpace *channel_space);
GeglChannelSpace* 
                gegl_channel_space_instance           (gchar *channel_space_name);

GeglChannelSpaceType 
                gegl_channel_space_channel_space_type (GeglChannelSpace * self);
gchar *         gegl_channel_space_name               (GeglChannelSpace * self);
gint            gegl_channel_space_bits               (GeglChannelSpace * self);
gboolean        gegl_channel_space_is_channel_data    (GeglChannelSpace * self);

void            gegl_channel_space_convert_to_float   (GeglChannelSpace * self, 
                                                       gfloat * dest, 
                                                       gpointer src, 
                                                       gint num);
void            gegl_channel_space_convert_from_float (GeglChannelSpace * self, 
                                                       gpointer dest, 
                                                       gfloat * src, 
                                                       gint num);

void            gegl_channel_space_convert_value_to_float (GeglChannelSpace * self, 
                                                       GValue * dest, 
                                                       GValue * src);

void            gegl_channel_space_convert_value_from_float (GeglChannelSpace * self, 
                                                        GValue * dest, 
                                                        GValue * src);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
