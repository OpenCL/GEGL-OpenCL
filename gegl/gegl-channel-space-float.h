#ifndef __GEGL_CHANNEL_SPACE_FLOAT_H__
#define __GEGL_CHANNEL_SPACE_FLOAT_H__

#include "gegl-channel-space.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHANNEL_SPACE_FLOAT               (gegl_channel_space_float_get_type ())
#define GEGL_CHANNEL_SPACE_FLOAT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNEL_SPACE_FLOAT, GeglChannelSpaceFloat))
#define GEGL_CHANNEL_SPACE_FLOAT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNEL_SPACE_FLOAT, GeglChannelSpaceFloatClass))
#define GEGL_IS_CHANNEL_SPACE_FLOAT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNEL_SPACE_FLOAT))
#define GEGL_IS_CHANNEL_SPACE_FLOAT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNEL_SPACE_FLOAT))
#define GEGL_CHANNEL_SPACE_FLOAT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNEL_SPACE_FLOAT, GeglChannelSpaceFloatClass))


typedef struct _GeglChannelSpaceFloat GeglChannelSpaceFloat;
struct _GeglChannelSpaceFloat 
{
   GeglChannelSpace channel_space;
};


typedef struct _GeglChannelSpaceFloatClass GeglChannelSpaceFloatClass;
struct _GeglChannelSpaceFloatClass 
{
  GeglChannelSpaceClass channel_space_class;
};


GType           gegl_channel_space_float_get_type       (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
