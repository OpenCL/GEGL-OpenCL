#include    "gegl-channel-param-specs.h"
#include    "gegl-channel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-image-buffer.h"
#include    "gegl-color-model.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-data-space.h"

GType GEGL_TYPE_PARAM_CHANNEL = 0;
GType GEGL_TYPE_PARAM_CHANNEL_UINT8 = 0;
GType GEGL_TYPE_PARAM_CHANNEL_FLOAT = 0;

/* --- channel param spec functions --- */

static void 
param_spec_channel_uint8_set_default (GParamSpec *pspec,
                              GValue     *value)
{
  GeglParamSpecChannelUInt8 *spec = GEGL_PARAM_SPEC_CHANNEL_UINT8 (pspec);
  value->data[1].v_uint = spec->default_value;
}

static gboolean
param_spec_channel_uint8_validate (GParamSpec *pspec,
                                   GValue     *value)
{
  GeglParamSpecChannelUInt8 *spec = GEGL_PARAM_SPEC_CHANNEL_UINT8 (pspec);
  guint oval = value->data[1].v_uint;
  
  value->data[1].v_uint = CLAMP (value->data[1].v_uint, spec->minimum, spec->maximum);
  
  return value->data[1].v_uint != oval;
}

static void 
param_spec_channel_float_set_default (GParamSpec *pspec,
                                      GValue     *value)
{
  GeglParamSpecChannelFloat *spec = GEGL_PARAM_SPEC_CHANNEL_FLOAT (pspec);
  value->data[1].v_float = spec->default_value;
}

static gboolean
param_spec_channel_float_validate (GParamSpec *pspec,
                                   GValue     *value)
{
  GeglParamSpecChannelFloat *spec = GEGL_PARAM_SPEC_CHANNEL_FLOAT (pspec);
  gfloat oval = value->data[1].v_float;
  
  value->data[1].v_float = CLAMP (value->data[1].v_float, spec->minimum, spec->maximum);
  
  return value->data[1].v_float != oval;
}

static gboolean
param_spec_channel_validate (GParamSpec *pspec,
                             GValue     *value)
{
  return FALSE;
}

void
gegl_channel_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecChannel),        /* instance_size */
        16,                                   /* n_preallocs */
        NULL,                                 /* instance_init */
        0x0,                                  /* value_type */
        NULL,                                 /* finalize */
        NULL,                                 /* value_set_default */
        param_spec_channel_validate,          /* value_validate */
        NULL,                                 /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_CHANNEL;
    GEGL_TYPE_PARAM_CHANNEL = 
      g_param_type_register_static ("GeglParamChannel", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_CHANNEL == 
              g_type_from_name("GeglParamChannel"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecChannelUInt8),   /* instance_size */
        16,                                   /* n_preallocs */
        NULL,                                 /* instance_init */
        0x0,                                  /* value_type */
        NULL,                                 /* finalize */
        param_spec_channel_uint8_set_default, /* value_set_default */
        param_spec_channel_uint8_validate,    /* value_validate */
        NULL,                                 /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_CHANNEL_UINT8;
    GEGL_TYPE_PARAM_CHANNEL_UINT8 = 
      g_param_type_register_static ("GeglParamChannelUInt8", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_CHANNEL_UINT8 == 
              g_type_from_name("GeglParamChannelUInt8"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecChannelFloat),  /* instance_size */
        16,                                  /* n_preallocs */
        NULL,                                /* instance_init */
        0x0,                                 /* value_type */
        NULL,                                /* finalize */
        param_spec_channel_float_set_default,/* value_set_default */
        param_spec_channel_float_validate,   /* value_validate */
        NULL,                                /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_CHANNEL_FLOAT;
    GEGL_TYPE_PARAM_CHANNEL_FLOAT = 
      g_param_type_register_static ("GeglParamChannelFloat", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_CHANNEL_FLOAT == 
              g_type_from_name("GeglParamChannelFloat"));
  }
}

GParamSpec*
gegl_param_spec_channel (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         GParamFlags  flags)
{
  GeglParamSpecChannel *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_CHANNEL, 
                                 name, nick, blurb, flags);

  return G_PARAM_SPEC (spec);
}

GParamSpec*
gegl_param_spec_channel_uint8 (const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               guint8 minimum,
                               guint8 maximum,
                               guint8 default_value,
                               GParamFlags  flags)
{
  GeglParamSpecChannelUInt8 *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_CHANNEL_UINT8, 
                                 name, nick, blurb, flags);

  spec->minimum = minimum;
  spec->maximum = maximum;
  spec->default_value = default_value;

  return G_PARAM_SPEC (spec);
}

GParamSpec*
gegl_param_spec_channel_float (const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               gfloat minimum,
                               gfloat maximum,
                               gfloat default_value,
                               GParamFlags  flags)
{
  GeglParamSpecChannelFloat *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_CHANNEL_FLOAT, 
                                name, nick, blurb, flags);

  spec->minimum = minimum;
  spec->maximum = maximum;
  spec->default_value = default_value;

  return G_PARAM_SPEC (spec);
}
