#ifndef __GEGL_OBJECT_H__
#define __GEGL_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-utils.h"

#define GEGL_TYPE_OBJECT               (gegl_object_get_type ())
#define GEGL_OBJECT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OBJECT, GeglObject))
#define GEGL_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OBJECT, GeglObjectClass))
#define GEGL_IS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OBJECT))
#define GEGL_IS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OBJECT))
#define GEGL_OBJECT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OBJECT, GeglObjectClass))

#ifndef __TYPEDEF_GEGL_OBJECT__
#define __TYPEDEF_GEGL_OBJECT__
typedef struct _GeglObject GeglObject;
#endif
struct _GeglObject {
       GObject __parent__;

       /*< private >*/
       gchar * name;
       gboolean constructed;  
       gint testflag; 
};

typedef struct _GeglObjectClass GeglObjectClass;
struct _GeglObjectClass {
       GObjectClass __parent__;
};

GType         gegl_object_get_type          (void); 
void          gegl_object_add_interface     (GeglObject * self, 
                                             const gchar * interface_name, 
                                             gpointer interface);
gpointer      gegl_object_query_interface   (GeglObject * self, 
                                             const gchar * interface_name);
void          gegl_object_set_name          (GeglObject * self, 
                                             const gchar * name);
G_CONST_RETURN gchar* 
              gegl_object_get_name          (GeglObject * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
