#ifndef __GEGL_MOCK_PROPERTIES_FILTER_H__
#define __GEGL_MOCK_PROPERTIES_FILTER_H__

#include "gegl-filter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_MOCK_PROPERTIES_FILTER               (gegl_mock_properties_filter_get_type ())
#define GEGL_MOCK_PROPERTIES_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_PROPERTIES_FILTER, GeglMockPropertiesFilter))
#define GEGL_MOCK_PROPERTIES_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_PROPERTIES_FILTER, GeglMockPropertiesFilterClass))
#define GEGL_IS_MOCK_PROPERTIES_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_PROPERTIES_FILTER))
#define GEGL_IS_MOCK_PROPERTIES_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_PROPERTIES_FILTER))
#define GEGL_MOCK_PROPERTIES_FILTER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_PROPERTIES_FILTER, GeglMockPropertiesFilterClass))

typedef struct _GeglMockPropertiesFilter GeglMockPropertiesFilter;
struct _GeglMockPropertiesFilter 
{
   GeglFilter filter;

   /*< private >*/
   gfloat glib_float;
   gfloat glib_int;
};

typedef struct _GeglMockPropertiesFilterClass GeglMockPropertiesFilterClass;
struct _GeglMockPropertiesFilterClass 
{
       GeglFilterClass filter_class;
};

GType         gegl_mock_properties_filter_get_type          (void); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
