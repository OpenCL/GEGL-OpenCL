#ifndef __GEGL_COMPONENT_STORAGE_H__
#define __GEGL_COMPONENT_STORAGE_H__

#include "gegl-storage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_COMPONENT_STORAGE               (gegl_component_storage_get_type ())
#define GEGL_COMPONENT_STORAGE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMPONENT_STORAGE, GeglComponentStorage))
#define GEGL_COMPONENT_STORAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMPONENT_STORAGE, GeglComponentStorageClass))
#define GEGL_IS_COMPONENT_STORAGE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMPONENT_STORAGE))
#define GEGL_IS_COMPONENT_STORAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMPONENT_STORAGE))
#define GEGL_COMPONENT_STORAGE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMPONENT_STORAGE, GeglComponentStorageClass))

typedef struct _GeglComponentStorage  GeglComponentStorage;
struct _GeglComponentStorage 
{
    GeglStorage storage;

   /*< private >*/
    gint num_banks;         
    gint pixel_stride_bytes;
    gint row_stride_bytes;
};

typedef struct _GeglComponentStorageClass GeglComponentStorageClass;
struct _GeglComponentStorageClass 
{
    GeglStorageClass storage_class;
};

GType           gegl_component_storage_get_type            (void);

gint            gegl_component_storage_num_banks           (GeglComponentStorage * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
