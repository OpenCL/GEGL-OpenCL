#ifndef __GEGL_FILTER_H__
#define __GEGL_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-op.h"

#ifndef __TYPEDEF_GEGL_SAMPLED_IMAGE__
#define __TYPEDEF_GEGL_SAMPLED_IMAGE__
typedef struct _GeglSampledImage GeglSampledImage;
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
   GeglOp __parent__;

   /*< private >*/
   GeglOp * root;
   GeglRect roi;
   GeglSampledImage *image;
};

typedef struct _GeglFilterClass GeglFilterClass;
struct _GeglFilterClass 
{
   GeglOpClass __parent__;
};

GType     gegl_filter_get_type         (void);
GeglOp *  gegl_filter_get_root         (GeglFilter * self); 
void      gegl_filter_set_root         (GeglFilter * self, 
                                        GeglOp *root);
void      gegl_filter_get_roi          (GeglFilter *self, 
                                        GeglRect *roi);
void      gegl_filter_set_roi          (GeglFilter *self, 
                                        GeglRect *roi);
GeglSampledImage*
          gegl_filter_get_image        (GeglFilter * self);
void      gegl_filter_set_image        (GeglFilter * self, 
                                        GeglSampledImage *image);

GList *   gegl_filter_get_input_values (GeglFilter *self, 
                                        GeglOp *op);

void gegl_filter_validate_input_values (GeglFilter *self, 
                                        GeglOp * op, 
                                        GList *input_values);
void gegl_filter_validate_output_values (GeglFilter *self, 
                                         GeglOp * op, 
                                         GList *output_values);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
