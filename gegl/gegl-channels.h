#ifndef __GEGL_CHANNELS_H__
#define __GEGL_CHANNELS_H__

#include "gegl-multi-image-op.h"
#include "gegl-scanline-processor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHANNELS               (gegl_channels_get_type ())
#define GEGL_CHANNELS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNELS, GeglChannels))
#define GEGL_CHANNELS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNELS, GeglChannelsClass))
#define GEGL_IS_CHANNELS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNELS))
#define GEGL_IS_CHANNELS_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNELS))
#define GEGL_CHANNELS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNELS, GeglChannelsClass))

typedef struct _GeglChannels GeglChannels;
struct _GeglChannels 
{
  GeglMultiImageOp multi_image_op;

  /*< private >*/
  GeglScanlineProcessor * scanline_processor;
};

typedef struct _GeglChannelsClass GeglChannelsClass;
struct _GeglChannelsClass 
{
  GeglMultiImageOpClass multi_image_op_class;
};

GType           gegl_channels_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
