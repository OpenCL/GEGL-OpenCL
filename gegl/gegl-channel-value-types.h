#ifndef __GEGL_CHANNEL_VALUE_TYPES_H__
#define __GEGL_CHANNEL_VALUE_TYPES_H__

#include <glib-object.h>
#include "gegl-types.h"

/* --- derived channel types --- */
extern GType GEGL_TYPE_CHANNEL_FLOAT;
extern GType GEGL_TYPE_CHANNEL_UINT8;

/* --- type macros --- */
#define G_VALUE_HOLDS_CHANNEL_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_CHANNEL_UINT8))
#define G_VALUE_HOLDS_CHANNEL_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_CHANNEL_FLOAT))

typedef struct _ChannelValueInfo ChannelValueInfo;
struct _ChannelValueInfo
{
  gchar * channel_space_name;
  gint bits_per_channel;
};

/* --- prototypes --- */
void gegl_channel_value_types_init (void);
void gegl_channel_value_transform_init(void);

void g_value_set_channel_uint8 (GValue *value, guint8  v_uint8);
guint8 g_value_get_channel_uint8 (const GValue *value);

void g_value_set_channel_float (GValue *value, gfloat  v_float);
gfloat g_value_get_channel_float (const GValue *value);

void g_value_channel_print(GValue * value);

#endif /* __GEGL_CHANNEL_VALUE_TYPES_H__ */
