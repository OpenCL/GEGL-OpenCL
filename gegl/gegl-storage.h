#ifndef __GEGL_STORAGE_H__
#define __GEGL_STORAGE_H__

#include "gegl-object.h"
#include "gegl-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_STORAGE               (gegl_storage_get_type ())
#define GEGL_STORAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_STORAGE, GeglStorage))
#define GEGL_STORAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STORAGE, GeglStorageClass))
#define GEGL_IS_STORAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_STORAGE))
#define GEGL_IS_STORAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STORAGE))
#define GEGL_STORAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STORAGE, GeglStorageClass))

typedef struct _GeglStorage  GeglStorage;
struct _GeglStorage 
{
    GeglObject object;

   /*< private >*/
    gint data_type_bytes; 
    gint num_bands;         
    gint width;
    gint height;
};

typedef struct _GeglStorageClass GeglStorageClass;
struct _GeglStorageClass 
{
    GeglObjectClass object_class;

    GeglBuffer*(*create_buffer) (GeglStorage *self);
};

GType           gegl_storage_get_type            (void);

gint            gegl_storage_data_type_bytes     (GeglStorage * self);
gint            gegl_storage_num_bands           (GeglStorage * self);
gint            gegl_storage_width               (GeglStorage * self);
gint            gegl_storage_height              (GeglStorage * self);
GeglBuffer* gegl_storage_create_buffer  (GeglStorage * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
