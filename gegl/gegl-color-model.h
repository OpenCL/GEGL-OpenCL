#ifndef __GEGL_COLOR_MODEL_H__
#define __GEGL_COLOR_MODEL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_COLOR_SPACE__
#define __TYPEDEF_GEGL_COLOR_SPACE__
typedef struct _GeglColorSpace  GeglColorSpace;
#endif

#ifndef __TYPEDEF_GEGL_DATA_SPACE__
#define __TYPEDEF_GEGL_DATA_SPACE__
typedef struct _GeglDataSpace  GeglDataSpace;
#endif

#ifndef __TYPEDEF_GEGL_STORAGE__
#define __TYPEDEF_GEGL_STORAGE__
typedef struct _GeglStorage  GeglStorage;
#endif

#define GEGL_TYPE_COLOR_MODEL               (gegl_color_model_get_type ())
#define GEGL_COLOR_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL, GeglColorModel))
#define GEGL_COLOR_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))
#define GEGL_IS_COLOR_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL))
#define GEGL_IS_COLOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL))
#define GEGL_COLOR_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))


#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif
struct _GeglColorModel 
{
   GeglObject object;

   /*< private >*/
   GeglColorSpace * color_space;
   GeglDataSpace *  data_space;

   gint num_channels;                    
   gint num_colors;                     

   gint *bits_per_channel;
   gint bits_per_pixel;

   gchar * name;       
   gchar ** channel_names;

   gboolean has_alpha;                   
   gint alpha_channel;

   gboolean has_z;
   gint z_channel;
};


typedef struct _GeglColorModelClass GeglColorModelClass;
struct _GeglColorModelClass 
{
   GeglObjectClass object_class;
   
   GeglStorage* (*create_storage)(GeglColorModel *self, gint w, gint h);
};


GType           gegl_color_model_get_type       (void);

gboolean        gegl_color_model_register       (GeglColorModel *color_model);
GeglColorModel *  gegl_color_model_instance       (gchar * color_model_name);

GeglColorSpace* gegl_color_model_color_space    (GeglColorModel * self);
GeglDataSpace*  gegl_color_model_data_space     (GeglColorModel * self);

gint            gegl_color_model_num_channels   (GeglColorModel * self);
gint            gegl_color_model_num_colors     (GeglColorModel * self);

gint*           gegl_color_model_bits_per_channel (GeglColorModel * self);
gint            gegl_color_model_bits_per_pixel (GeglColorModel * self);

gchar*          gegl_color_model_name            (GeglColorModel * self);
gchar**         gegl_color_model_channel_names   (GeglColorModel * self);

gboolean        gegl_color_model_has_alpha      (GeglColorModel * self);
void            gegl_color_model_set_has_alpha  (GeglColorModel * self, 
                                               gboolean has_alpha);
gint            gegl_color_model_alpha_channel  (GeglColorModel * self);

gboolean        gegl_color_model_has_z          (GeglColorModel * self);
void            gegl_color_model_set_has_z      (GeglColorModel * self, 
                                               gboolean has_z);
gint            gegl_color_model_z_channel      (GeglColorModel * self);

GeglStorage*    gegl_color_model_create_storage (GeglColorModel * self, 
                                               gint width, 
                                               gint height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
