#ifndef __GEGL_MOCK_FILTER_H__
#define __GEGL_MOCK_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-filter.h"

#define GEGL_TYPE_MOCK_FILTER               (gegl_mock_filter_get_type ())
#define GEGL_MOCK_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_FILTER, GeglMockFilter))
#define GEGL_MOCK_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_FILTER, GeglMockFilterClass))
#define GEGL_IS_MOCK_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_FILTER))
#define GEGL_IS_MOCK_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_FILTER))
#define GEGL_MOCK_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_FILTER, GeglMockFilterClass))

#ifndef __TYPEDEF_GEGL_MOCK_FILTER__
#define __TYPEDEF_GEGL_MOCK_FILTER__
typedef struct _GeglMockFilter GeglMockFilter;
#endif
struct _GeglMockFilter 
{
   GeglFilter filter;

   /*< private >*/
   gfloat glib_float;
   gfloat glib_int;

   GValue *channel;
   GValue *pixel;
};

typedef struct _GeglMockFilterClass GeglMockFilterClass;
struct _GeglMockFilterClass 
{
       GeglFilterClass filter_class;
};

GType         gegl_mock_filter_get_type          (void); 

void gegl_mock_filter_get_channel (GeglMockFilter * self, 
                                   GValue *channel);
void gegl_mock_filter_set_channel (GeglMockFilter * self, 
                                   GValue *pixel);

void gegl_mock_filter_get_pixel    (GeglMockFilter * self, 
                                    GValue *pixel);
void gegl_mock_filter_set_pixel    (GeglMockFilter * self, 
                                    GValue *pixel);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
