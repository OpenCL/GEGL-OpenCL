#ifndef __GEGL_CHANNEL_DATA_H__
#define __GEGL_CHANNEL_DATA_H__

#include "gegl-data.h"
#include "gegl-channel-space.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_CHANNEL_DATA               (gegl_channel_data_get_type ())
#define GEGL_CHANNEL_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNEL_DATA, GeglChannelData))
#define GEGL_CHANNEL_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNEL_DATA, GeglChannelDataClass))
#define GEGL_IS_CHANNEL_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNEL_DATA))
#define GEGL_IS_CHANNEL_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNEL_DATA))
#define GEGL_CHANNEL_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNEL_DATA, GeglChannelDataClass))

typedef struct _GeglChannelData GeglChannelData;
struct _GeglChannelData 
{
    GeglData data;

    /*< private >*/
    GeglChannelSpace *channel_space;
};

typedef struct _GeglChannelDataClass GeglChannelDataClass;
struct _GeglChannelDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_channel_data_get_type              (void);
GeglChannelSpace*  gegl_channel_data_get_channel_space        (GeglChannelData * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
