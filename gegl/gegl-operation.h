#ifndef __GEGL_FILTER_H__
#define __GEGL_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op.h"
#include "gegl-attributes.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_FILTER               (gegl_filter_get_type ())
#define GEGL_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILTER, GeglFilter))
#define GEGL_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILTER, GeglFilterClass))
#define GEGL_IS_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILTER))
#define GEGL_IS_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILTER))
#define GEGL_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILTER, GeglFilterClass))

#ifndef __TYPEDEF_GEGL_FILTER__
#define __TYPEDEF_GEGL_FILTER__
typedef struct _GeglFilter GeglFilter;
#endif
struct _GeglFilter 
{
   GeglOp op;

   /*< private >*/
};

typedef struct _GeglFilterClass GeglFilterClass;
struct _GeglFilterClass 
{
   GeglOpClass op_class;
                                    
   void (* evaluate)                (GeglFilter * self, 
                                     GList * attributes,
                                     GList * input_attributes);
   void (* prepare)                 (GeglFilter * self, 
                                     GList * attributes,
                                     GList * input_attributes);
   void (* process)                 (GeglFilter * self, 
                                     GList * attributes,
                                     GList * input_attributes);
   void (* finish)                  (GeglFilter * self, 
                                     GList * attributes,
                                     GList * input_attributes);

   void (* compute_need_rect)       (GeglFilter *self,
                                     GeglRect *input_need_rect,
                                     GeglRect *need_rect,
                                     gint i);
   void (* compute_have_rect)       (GeglFilter *self,
                                     GeglRect *have_rect,
                                     GList * input_have_rect); 
   GeglColorModel*
     (* compute_derived_color_model)(GeglFilter *self,
                                     GList * input_color_models); 
};

GType           gegl_filter_get_type            (void);
void            gegl_filter_compute_need_rect   (GeglFilter * self,
                                                 GeglRect * input_need_rect,
                                                 GeglRect * need_rect,
                                                 gint i);
void            gegl_filter_compute_have_rect   (GeglFilter * self,
                                                 GeglRect *have_rect,
                                                 GList * input_have_rects);
GeglColorModel*  
         gegl_filter_compute_derived_color_model(GeglFilter * self, 
                                                 GList * input_color_models);
void            gegl_filter_evaluate            (GeglFilter * self, 
                                                 GList * attributes,
                                                 GList * input_attributes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
