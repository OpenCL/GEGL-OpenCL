#ifndef __GEGL_CHANNEL_SPACE_UINT8_H__
#define __GEGL_CHANNEL_SPACE_UINT8_H__

#include "gegl-channel-space.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHANNEL_SPACE_UINT8               (gegl_channel_space_uint8_get_type ())
#define GEGL_CHANNEL_SPACE_UINT8(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNEL_SPACE_UINT8, GeglChannelSpaceU8))
#define GEGL_CHANNEL_SPACE_UINT8_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNEL_SPACE_UINT8, GeglChannelSpaceU8Class))
#define GEGL_IS_CHANNEL_SPACE_UINT8(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNEL_SPACE_UINT8))
#define GEGL_IS_CHANNEL_SPACE_UINT8_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNEL_SPACE_UINT8))
#define GEGL_CHANNEL_SPACE_UINT8_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNEL_SPACE_UINT8, GeglChannelSpaceU8Class))


typedef struct _GeglChannelSpaceU8 GeglChannelSpaceU8;
struct _GeglChannelSpaceU8 
{
   GeglChannelSpace channel_space;
};


typedef struct _GeglChannelSpaceU8Class GeglChannelSpaceU8Class;
struct _GeglChannelSpaceU8Class 
{
  GeglChannelSpaceClass channel_space_class;
};


GType           gegl_channel_space_uint8_get_type       (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
