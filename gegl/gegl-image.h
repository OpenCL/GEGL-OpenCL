#ifndef __GEGL_IMAGE_H__
#define __GEGL_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE_IMPL__
#define __TYPEDEF_GEGL_IMAGE_IMPL__
typedef struct _GeglImageImpl GeglImageImpl;
#endif

#define GEGL_TYPE_IMAGE               (gegl_image_get_type ())
#define GEGL_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE, GeglImage))
#define GEGL_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_IMAGE, GeglImageClass))
#define GEGL_IS_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE))
#define GEGL_IS_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_IMAGE))
#define GEGL_IMAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_IMAGE, GeglImageClass))

#ifndef __TYPEDEF_GEGL_IMAGE__
#define __TYPEDEF_GEGL_IMAGE__
typedef struct _GeglImage GeglImage;
#endif
struct _GeglImage {
   GeglOp __parent__;

   /*< private >*/
   GeglRect have_rect;
   GeglRect need_rect;
   GeglRect result_rect;
   GeglColorModel * derived_color_model;
   gboolean color_model_is_derived;
   GeglImageImpl * dest;
};

typedef struct _GeglImageClass GeglImageClass;
struct _GeglImageClass {
   GeglOpClass __parent__;

   void (* compute_derived_color_model)     (GeglImage * self, 
                                             GList * inputs);
   void (* compute_result_rect)             (GeglImage * self, 
                                             GeglRect * result_rect,  
                                             GeglRect * need_rect,
                                             GeglRect * have_rect);
   void (* compute_have_rect)               (GeglImage * self, 
                                             GeglRect * have_rect,   
                                             GList * inputs_have_rects);
   void (* compute_preimage)                (GeglImage * self, 
                                             GeglRect * preimage, 
                                             GeglRect * rect, 
                                             gint i);
};

GType            gegl_image_get_type                     (void);
GeglColorModel*  gegl_image_color_model                  (GeglImage * self);
void             gegl_image_set_color_model              (GeglImage * self, 
                                                             GeglColorModel * cm);

GeglColorModel*  gegl_image_derived_color_model          (GeglImage * self);
void             gegl_image_set_derived_color_model      (GeglImage * self, 
                                                             GeglColorModel * cm);

void             gegl_image_compute_derived_color_model  (GeglImage * self, 
                                                             GList * input_color_models);

void             gegl_image_compute_have_rect (GeglImage * self, 
                                                  GeglRect * have_rect, 
                                                  GList * inputs_have_rects);

void             gegl_image_compute_result_rect (GeglImage * self, 
                                                    GeglRect * result_rect, 
                                                    GeglRect * need_rect, 
                                                    GeglRect * have_rect);

void             gegl_image_get_have_rect    (GeglImage   * self, 
                                                 GeglRect * have_rect);
void             gegl_image_set_have_rect    (GeglImage   * self, 
                                                 GeglRect * have_rect);
void             gegl_image_get_result_rect  (GeglImage   * self, 
                                                 GeglRect * result_rect);
void             gegl_image_set_result_rect  (GeglImage   * self, 
                                                 GeglRect * result_rect);

void             gegl_image_compute_preimage (GeglImage * self, 
                                                 GeglRect * preimage, 
                                                 GeglRect * rect, 
                                                 gint i);

void             gegl_image_get_need_rect    (GeglImage   * self, 
                                                 GeglRect * need_rect);
void             gegl_image_set_need_rect    (GeglImage   * self, 
                                                 GeglRect * need_rect);

GeglImageImpl*   gegl_image_get_dest         (GeglImage *self);

void             gegl_image_set_dest         (GeglImage *self,
                                                 GeglImageImpl *dest);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
