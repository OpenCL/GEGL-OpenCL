#ifndef __GEGL_MEM_BUFFER_H__
#define __GEGL_MEM_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-buffer.h"

#define GEGL_TYPE_MEM_BUFFER               (gegl_mem_buffer_get_type ())
#define GEGL_MEM_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MEM_BUFFER, GeglMemBuffer))
#define GEGL_MEM_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MEM_BUFFER, GeglMemBufferClass))
#define GEGL_IS_MEM_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MEM_BUFFER))
#define GEGL_IS_MEM_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MEM_BUFFER))
#define GEGL_MEM_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MEM_BUFFER, GeglMemBufferClass))

#ifndef __TYPEDEF_GEGL_MEM_BUFFER__
#define __TYPEDEF_GEGL_MEM_BUFFER__
typedef struct _GeglMemBuffer GeglMemBuffer;
#endif
struct _GeglMemBuffer {
   GeglBuffer __parent__;
};

typedef struct _GeglMemBufferClass GeglMemBufferClass;
struct _GeglMemBufferClass {
   GeglBufferClass __parent__;
};

GType              gegl_mem_buffer_get_type   (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
