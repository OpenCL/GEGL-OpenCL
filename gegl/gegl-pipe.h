#ifndef __GEGL_PIPE_H__
#define __GEGL_PIPE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-filter.h"
#include "gegl-scanline-processor.h"

#define GEGL_TYPE_PIPE               (gegl_pipe_get_type ())
#define GEGL_PIPE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PIPE, GeglPipe))
#define GEGL_PIPE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PIPE, GeglPipeClass))
#define GEGL_IS_PIPE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PIPE))
#define GEGL_IS_PIPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PIPE))
#define GEGL_PIPE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PIPE, GeglPipeClass))

#ifndef __TYPEDEF_GEGL_PIPE__
#define __TYPEDEF_GEGL_PIPE__
typedef struct _GeglPipe GeglPipe;
#endif
struct _GeglPipe 
{
    GeglFilter filter;

    /*< private >*/
    GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglPipeClass GeglPipeClass;
struct _GeglPipeClass 
{
    GeglFilterClass filter_class;

    GeglScanlineFunc (*get_scanline_func)   (GeglPipe *self,
                                             GeglColorSpace space,
                                             GeglChannelDataType type);
};

GType           gegl_pipe_get_type           (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
