#ifndef __GEGL_SCANLINE_PROCESSOR_H__
#define __GEGL_SCANLINE_PROCESSOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"
#include "gegl-filter.h"
#include "gegl-image-data-iterator.h"

typedef void (*GeglScanlineFunc)(GeglFilter *op,
                                 GeglImageDataIterator ** iters, 
                                 gint width);


#ifndef __TYPEDEF_GEGL_ATTRIBUTES__
#define __TYPEDEF_GEGL_ATTRIBUTES__
typedef struct _GeglAttributes GeglAttributes;
#endif

#define GEGL_TYPE_SCANLINE_PROCESSOR               (gegl_scanline_processor_get_type ())
#define GEGL_SCANLINE_PROCESSOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SCANLINE_PROCESSOR, GeglScanlineProcessor))
#define GEGL_SCANLINE_PROCESSOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SCANLINE_PROCESSOR, GeglScanlineProcessorClass))
#define GEGL_IS_SCANLINE_PROCESSOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SCANLINE_PROCESSOR))
#define GEGL_IS_SCANLINE_PROCESSOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SCANLINE_PROCESSOR))
#define GEGL_SCANLINE_PROCESSOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SCANLINE_PROCESSOR, GeglScanlineProcessorClass))

#ifndef __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
#define __TYPEDEF_GEGL_SCANLINE_PROCESSOR__
typedef struct _GeglScanlineProcessor GeglScanlineProcessor;
#endif

struct _GeglScanlineProcessor 
{
   GeglObject object;

   /*< private >*/
   GeglScanlineFunc func;
   GeglFilter *op;
};

typedef struct _GeglScanlineProcessorClass GeglScanlineProcessorClass;
struct _GeglScanlineProcessorClass 
{
   GeglObjectClass object_class;
};

GType gegl_scanline_processor_get_type   (void);

void gegl_scanline_processor_process(GeglScanlineProcessor  *self,
                                     GeglAttributes * attributes,
                                     GList * input_attributes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
