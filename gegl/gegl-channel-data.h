#ifndef __GEGL_CHANNEL_DATA_H__
#define __GEGL_CHANNEL_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-data.h"

#ifndef __TYPEDEF_GEGL_DATA_SPACE__
#define __TYPEDEF_GEGL_DATA_SPACE__
typedef struct _GeglDataSpace  GeglDataSpace;
#endif

#define GEGL_TYPE_CHANNEL_DATA               (gegl_channel_data_get_type ())
#define GEGL_CHANNEL_DATA(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CHANNEL_DATA, GeglChannelData))
#define GEGL_CHANNEL_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CHANNEL_DATA, GeglChannelDataClass))
#define GEGL_IS_CHANNEL_DATA(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CHANNEL_DATA))
#define GEGL_IS_CHANNEL_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CHANNEL_DATA))
#define GEGL_CHANNEL_DATA_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CHANNEL_DATA, GeglChannelDataClass))

#ifndef __TYPEDEF_GEGL_CHANNEL_DATA__
#define __TYPEDEF_GEGL_CHANNEL_DATA__
typedef struct _GeglChannelData GeglChannelData;
#endif
struct _GeglChannelData 
{
    GeglData data;

    /*< private >*/
    GeglDataSpace *data_space;
};

typedef struct _GeglChannelDataClass GeglChannelDataClass;
struct _GeglChannelDataClass 
{
    GeglDataClass data_class;
};

GType           gegl_channel_data_get_type              (void);
GeglDataSpace*  gegl_channel_data_get_data_space        (GeglChannelData * self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
