#ifndef __GEGL_DATA_H__
#define __GEGL_DATA_H__

#include "gegl-object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_DATA               (gegl_data_get_type ())
#define GEGL_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DATA, GeglData))
#define GEGL_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DATA, GeglDataClass))
#define GEGL_IS_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DATA))
#define GEGL_IS_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DATA))
#define GEGL_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DATA, GeglDataClass))

typedef struct _GeglData GeglData;
struct _GeglData 
{
    GeglObject object;

    /*< private >*/
    GValue *value;
    GParamSpec *param_spec;
    gchar *name;
};

typedef struct _GeglDataClass GeglDataClass;
struct _GeglDataClass 
{
    GeglObjectClass object_class;
};

GType           gegl_data_get_type              (void);
GValue*         gegl_data_get_value             (GeglData * self);

void            gegl_data_copy_value            (GeglData * self, const GValue* value);

void            gegl_data_set_name              (GeglData * self, const gchar * name);
G_CONST_RETURN gchar* 
                gegl_data_get_name              (GeglData * self);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
