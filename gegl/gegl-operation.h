#ifndef __GEGL_FILTER_H__
#define __GEGL_FILTER_H__

#include "gegl-op.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_FILTER               (gegl_filter_get_type ())
#define GEGL_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILTER, GeglFilter))
#define GEGL_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILTER, GeglFilterClass))
#define GEGL_IS_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILTER))
#define GEGL_IS_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILTER))
#define GEGL_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILTER, GeglFilterClass))

typedef struct _GeglFilter GeglFilter;
struct _GeglFilter 
{
   GeglOp op;

   /*< private >*/
};

typedef struct _GeglFilterClass GeglFilterClass;
struct _GeglFilterClass 
{
   GeglOpClass op_class;
                                    
   void (* prepare)                 (GeglFilter * self); 
   void (* process)                 (GeglFilter * self); 
   void (* finish)                  (GeglFilter * self); 

   void (* validate_inputs)         (GeglFilter *self,
                                     GArray *collected_data);
   void (* validate_outputs)        (GeglFilter *self);
};

GType           gegl_filter_get_type            (void);
void            gegl_filter_evaluate            (GeglFilter * self); 
void            gegl_filter_validate_inputs     (GeglFilter * self, 
                                                 GArray * collected_data);
void            gegl_filter_validate_outputs    (GeglFilter * self);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
