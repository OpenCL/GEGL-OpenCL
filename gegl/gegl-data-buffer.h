#ifndef __GEGL_DATA_BUFFER_H__
#define __GEGL_DATA_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#define GEGL_TYPE_DATA_BUFFER               (gegl_data_buffer_get_type ())
#define GEGL_DATA_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DATA_BUFFER, GeglDataBuffer))
#define GEGL_DATA_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DATA_BUFFER, GeglDataBufferClass))
#define GEGL_IS_DATA_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DATA_BUFFER))
#define GEGL_IS_DATA_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DATA_BUFFER))
#define GEGL_DATA_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DATA_BUFFER, GeglDataBufferClass))

#ifndef __TYPEDEF_GEGL_DATA_BUFFER__
#define __TYPEDEF_GEGL_DATA_BUFFER__
typedef struct _GeglDataBuffer  GeglDataBuffer;
#endif
struct _GeglDataBuffer 
{
    GeglObject object;

   /*< private >*/
    gpointer * banks; 
    gint       bytes_per_bank;     
    gint num_banks;         
};

typedef struct _GeglDataBufferClass GeglDataBufferClass;
struct _GeglDataBufferClass 
{
    GeglObjectClass object_class;
};

GType           gegl_data_buffer_get_type            (void);

gint            gegl_data_buffer_num_banks     (GeglDataBuffer * self);
gint            gegl_data_buffer_bytes_per_bank(GeglDataBuffer * self);
gint            gegl_data_buffer_total_bytes   (GeglDataBuffer * self);
gpointer *      gegl_data_buffer_banks_data     (GeglDataBuffer * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
