#ifndef __GEGL_MOCK_PARAM_FILTER_H__
#define __GEGL_MOCK_PARAM_FILTER_H__

#include "gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_PARAM_FILTER               (gegl_mock_param_filter_get_type ())
#define GEGL_MOCK_PARAM_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_PARAM_FILTER, GeglMockParamFilter))
#define GEGL_MOCK_PARAM_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_PARAM_FILTER, GeglMockParamFilterClass))
#define GEGL_IS_MOCK_PARAM_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_PARAM_FILTER))
#define GEGL_IS_MOCK_PARAM_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_PARAM_FILTER))
#define GEGL_MOCK_PARAM_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_PARAM_FILTER, GeglMockParamFilterClass))

typedef struct _GeglMockParamFilter GeglMockParamFilter;
struct _GeglMockParamFilter 
{
   GeglFilter filter;

   /*< private >*/
   gfloat glib_float;
   gfloat glib_int;

   GValue *channel;
   GValue *pixel;
};

typedef struct _GeglMockParamFilterClass GeglMockParamFilterClass;
struct _GeglMockParamFilterClass 
{
       GeglFilterClass filter_class;
};

GType         gegl_mock_param_filter_get_type          (void); 

void gegl_mock_param_filter_get_channel (GeglMockParamFilter * self, 
                                   GValue *channel);
void gegl_mock_param_filter_set_channel (GeglMockParamFilter * self, 
                                   GValue *pixel);

void gegl_mock_param_filter_get_pixel    (GeglMockParamFilter * self, 
                                    GValue *pixel);
void gegl_mock_param_filter_set_pixel    (GeglMockParamFilter * self, 
                                    GValue *pixel);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
