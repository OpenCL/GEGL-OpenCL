#ifndef __GEGL_BUFFER_H__
#define __GEGL_BUFFER_H__

#include "gegl-object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_BUFFER               (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

typedef struct _GeglBuffer  GeglBuffer;
struct _GeglBuffer 
{
    GeglObject object;

   /*< private >*/
    gpointer * banks; 
    gint       bytes_per_bank;     
    gint num_banks;         
};

typedef struct _GeglBufferClass GeglBufferClass;
struct _GeglBufferClass 
{
    GeglObjectClass object_class;
};

GType           gegl_buffer_get_type            (void);

gint            gegl_buffer_num_banks     (GeglBuffer * self);
gint            gegl_buffer_bytes_per_bank(GeglBuffer * self);
gint            gegl_buffer_total_bytes   (GeglBuffer * self);
gpointer *      gegl_buffer_banks_data     (GeglBuffer * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
