#ifndef __GEGL_COLOR_MODEL_H__
#define __GEGL_COLOR_MODEL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_COLOR__
#define __TYPEDEF_GEGL_COLOR__
typedef struct _GeglColor GeglColor;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

typedef void (*GeglConvertFunc)(GeglColorModel *,
                    GeglColorModel *,
                    guchar **,
                    guchar **,
                    gint);


#define GEGL_TYPE_COLOR_MODEL               (gegl_color_model_get_type ())
#define GEGL_COLOR_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL, GeglColorModel))
#define GEGL_COLOR_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))
#define GEGL_IS_COLOR_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL))
#define GEGL_IS_COLOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL))
#define GEGL_COLOR_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))


struct _GeglColorModel {
   GeglObject __parent__;

   /*< private >*/
   GeglColorSpace colorspace;            /*  */
   GeglChannelDataType data_type;        /*  */
   gint bytes_per_channel;               /*  */
   gint bytes_per_pixel;                 /*  */
   gint num_channels;                    /*  */
   gint num_colors;                      /*  */
   gboolean has_alpha;                   /*  */
   gint alpha_channel;                   /*  */
   gboolean is_premultiplied;            /*  */
   gboolean is_additive;                 /*  */
   gboolean is_subtractive;              /*  */
   const char ** channel_names;          /*  */
   const char * color_space_name;        /*  */
   const char * alpha_string;            /*  */
   const char * channel_data_type_name;  /*  */
};


typedef struct _GeglColorModelClass GeglColorModelClass;
struct _GeglColorModelClass {
   GeglObjectClass __parent__;

   GeglColorAlphaSpace (* color_alpha_space)              (GeglColorModel * self);
   GeglColorModelType  (* color_model_type)               (GeglColorModel * self);
   void                (* set_color)                      (GeglColorModel * self, 
                                                           GeglColor * color, 
                                                           GeglColorConstant constant);
   void                (* convert_to_xyz)                 (GeglColorModel * self, 
                                                           gfloat ** dest_data, 
                                                           guchar ** src_data, 
                                                           gint width);
   void                (* convert_from_xyz)               (GeglColorModel * self, 
                                                           guchar ** dest_data, 
                                                           gfloat ** src_data, 
                                                           gint width);
   gchar *             (* get_convert_interface_name)     (GeglColorModel * self);
};


gboolean              gegl_color_model_register           (gchar * color_model_name, 
                                                           GeglColorModel *color_model);
GeglColorModel *      gegl_color_model_instance           (gchar * color_model_name);


GType                 gegl_color_model_get_type           (void);
GeglColorSpace        gegl_color_model_color_space        (GeglColorModel * self);
GeglChannelDataType   gegl_color_model_data_type          (GeglColorModel * self);
gint                  gegl_color_model_bytes_per_channel  (GeglColorModel * self);
gint                  gegl_color_model_bytes_per_pixel    (GeglColorModel * self);
gint                  gegl_color_model_num_channels       (GeglColorModel * self);
gint                  gegl_color_model_num_colors         (GeglColorModel * self);
gboolean              gegl_color_model_has_alpha          (GeglColorModel * self);
gint                  gegl_color_model_alpha_channel      (GeglColorModel * self);
GeglColorAlphaSpace   gegl_color_model_color_alpha_space  (GeglColorModel * self);
GeglColorModelType    gegl_color_model_color_model_type   (GeglColorModel * self);
void                  gegl_color_model_set_color          (GeglColorModel * self,
                                                           GeglColor * color,
                                                           GeglColorConstant constant);
void                  gegl_color_model_convert_to_xyz     (GeglColorModel * self,
                                                           gfloat ** dest_data,
                                                           guchar ** src_data,
                                                           gint width);
void                  gegl_color_model_convert_from_xyz   (GeglColorModel * self,
                                                           guchar ** dest_data,
                                                           gfloat ** src_data,
                                                           gint width);
gchar *               gegl_color_model_get_convert_interface_name    (GeglColorModel * self);

void                  gegl_color_model_set_has_alpha       (GeglColorModel * self, 
                                                            gboolean has_alpha);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
