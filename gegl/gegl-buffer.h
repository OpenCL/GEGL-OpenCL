#ifndef __GEGL_BUFFER_H__
#define __GEGL_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib-object.h>
#include "gegl-object.h"

#define GEGL_TYPE_BUFFER               (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

#ifndef __TYPEDEF_GEGL_BUFFER__
#define __TYPEDEF_GEGL_BUFFER__
typedef struct _GeglBuffer  GeglBuffer;
#endif
struct _GeglBuffer {
   GeglObject __parent__;

   /*< private >*/
   gpointer * data_pointers; 
   gint bytes_per_buffer;     
   gint num_buffers;         
   gint ref_count;       
};

typedef struct _GeglBufferClass GeglBufferClass;
struct _GeglBufferClass {
   GeglObjectClass __parent__;

   void (* unalloc_data) (GeglBuffer * self);
   void (* alloc_data) (GeglBuffer * self);
   void (* ref) (GeglBuffer * self);
   void (* unref) (GeglBuffer * self);
};

GType         gegl_buffer_get_type             (void);
gint          gegl_buffer_get_bytes_per_buffer (GeglBuffer * self);
gint          gegl_buffer_get_num_buffers      (GeglBuffer * self);
gpointer *    gegl_buffer_get_data_pointers    (GeglBuffer * self);
void          gegl_buffer_unalloc_data         (GeglBuffer * self);
void          gegl_buffer_alloc_data           (GeglBuffer * self);
void          gegl_buffer_ref                  (GeglBuffer * self);
void          gegl_buffer_unref                (GeglBuffer * self);
gint          gegl_buffer_get_ref_count        (GeglBuffer * self);

void          gegl_buffer_set_bytes_per_buffer (GeglBuffer * self, gint bytes_per_buffer);
void          gegl_buffer_set_num_buffers      (GeglBuffer * self, gint num_buffers);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
