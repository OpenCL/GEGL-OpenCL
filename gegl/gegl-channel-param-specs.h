#ifndef __GEGL_CHANNEL_PARAM_SPECS_H__
#define __GEGL_CHANNEL_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

extern GType GEGL_TYPE_PARAM_CHANNEL_UINT8;
extern GType GEGL_TYPE_PARAM_CHANNEL_FLOAT;

#define GEGL_IS_PARAM_SPEC_CHANNEL_UINT8(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_CHANNEL_UINT8))
#define GEGL_PARAM_SPEC_CHANNEL_UINT8(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_CHANNEL_UINT8, GeglParamSpecChannelUInt8))

#define GEGL_IS_PARAM_SPEC_CHANNEL_FLOAT(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_CHANNEL_FLOAT))
#define GEGL_PARAM_SPEC_CHANNEL_FLOAT(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_CHANNEL_FLOAT, GeglParamSpecChannelFloat))

typedef struct _GeglParamSpecChannelUInt8     GeglParamSpecChannelUInt8;
struct _GeglParamSpecChannelUInt8
{
  GParamSpec pspec;

  guint8 minimum;
  guint8 maximum;
  guint8 default_value;
};

typedef struct _GeglParamSpecChannelFloat     GeglParamSpecChannelFloat;
struct _GeglParamSpecChannelFloat
{
  GParamSpec pspec;

  gfloat minimum;
  gfloat maximum;
  gfloat default_value;
};

GParamSpec*  gegl_param_spec_channel_uint8   (const gchar     *name,
                                              const gchar     *nick,
                                              const gchar     *blurb,
                                              guint8 min,
                                              guint8 max,
                                              guint8 default_value,
                                              GParamFlags     flags);

GParamSpec*  gegl_param_spec_channel_float   (const gchar     *name,
                                              const gchar     *nick,
                                              const gchar     *blurb,
                                              gfloat min,
                                              gfloat max,
                                              gfloat default_value,
                                              GParamFlags     flags);

void gegl_channel_param_spec_types_init (void);

#endif /* __GEGL_CHANNEL_PARAM_SPECS_H__ */
