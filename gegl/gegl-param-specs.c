#include    "gegl-param-specs.h"
#include    "gegl-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-image-data.h"
#include    "gegl-color-model.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-data-space.h"

GType *gegl_param_spec_types = NULL;

/* --- param spec functions --- */

static void 
param_spec_uint8_set_default (GParamSpec *pspec,
                              GValue     *value)
{
  GeglParamSpecUInt8 *spec = GEGL_PARAM_SPEC_UINT8 (pspec);
  value->data[1].v_uint = spec->default_value;
}

static gboolean
param_spec_uint8_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GeglParamSpecUInt8 *spec = GEGL_PARAM_SPEC_UINT8 (pspec);
  guint oval = value->data[1].v_uint;
  
  value->data[1].v_uint = CLAMP (value->data[1].v_uint, spec->minimum, spec->maximum);
  
  return value->data[1].v_uint != oval;
}

static void 
param_spec_float_set_default (GParamSpec *pspec,
                              GValue     *value)
{
  GeglParamSpecFloat *spec = GEGL_PARAM_SPEC_FLOAT (pspec);
  value->data[1].v_float = spec->default_value;
}

static gboolean
param_spec_float_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GeglParamSpecFloat *spec = GEGL_PARAM_SPEC_FLOAT (pspec);
  gfloat oval = value->data[1].v_float;
  
  value->data[1].v_float = CLAMP (value->data[1].v_float, spec->minimum, spec->maximum);
  
  return value->data[1].v_float != oval;
}

static void 
param_spec_rgb_uint8_set_default (GParamSpec *pspec,
                                  GValue     *value)
{
  GeglParamSpecRgbUInt8 *spec = GEGL_PARAM_SPEC_RGB_UINT8 (pspec);
  guint8 *p = (guint8*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
}

static gboolean
param_spec_rgb_uint8_validate (GParamSpec *pspec,
                               GValue     *value)
{
  GeglParamSpecRgbUInt8 *spec = GEGL_PARAM_SPEC_RGB_UINT8 (pspec);
  guint8 *p = (guint8*)value->data[1].v_pointer;
  guint8 old_red = p[0];
  guint8 old_green = p[1];
  guint8 old_blue = p[2];
  
  p[0] = CLAMP (p[0], spec->min_red, spec->max_red);
  p[1] = CLAMP (p[1], spec->min_green, spec->max_green);
  p[2] = CLAMP (p[2], spec->min_blue, spec->max_blue);

  return (old_red != p[0]) || 
         (old_green != p[1]) || 
         (old_blue != p[2]);  
}

static void 
param_spec_rgb_float_set_default (GParamSpec *pspec,
                                  GValue     *value)
{
  GeglParamSpecRgbFloat *spec = GEGL_PARAM_SPEC_RGB_FLOAT (pspec);
  gfloat *p = (gfloat*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
}

static gboolean
param_spec_rgb_float_validate (GParamSpec *pspec,
                               GValue     *value)
{
  GeglParamSpecRgbFloat *spec = GEGL_PARAM_SPEC_RGB_FLOAT (pspec);
  gfloat *p = (gfloat*)value->data[1].v_pointer;
  gfloat old_red = p[0];
  gfloat old_green = p[1];
  gfloat old_blue = p[2];
  
  p[0] = CLAMP (p[0], spec->min_red, spec->max_red);
  p[1] = CLAMP (p[1], spec->min_green, spec->max_green);
  p[2] = CLAMP (p[2], spec->min_blue, spec->max_blue);

  return (old_red != p[0]) || 
         (old_green != p[1]) || 
         (old_blue != p[2]);  
}

static gboolean
param_spec_image_data_validate (GParamSpec *pspec,
                                GValue     *value)
{
  return FALSE;
}

void
gegl_param_spec_types_init (void)
{
  const guint n_types = 5;
  GType type, *spec_types, *spec_types_bound;

  gegl_param_spec_types = g_new0 (GType, n_types);
  spec_types = gegl_param_spec_types;
  spec_types_bound = gegl_param_spec_types + n_types;

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecUInt8),     /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        param_spec_uint8_set_default,    /* value_set_default */
        param_spec_uint8_validate,       /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_UINT8;
    type = g_param_type_register_static ("GeglParamUInt8", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_UINT8);
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecFloat),     /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        param_spec_float_set_default,    /* value_set_default */
        param_spec_float_validate,       /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_FLOAT;
    type = g_param_type_register_static ("GeglParamFloat", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_FLOAT);
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecRgbUInt8),  /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        param_spec_rgb_uint8_set_default,/* value_set_default */
        param_spec_rgb_uint8_validate,   /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_RGB_UINT8;
    type = g_param_type_register_static ("GeglParamRgbUInt8", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_RGB_UINT8);
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecRgbFloat),  /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        param_spec_rgb_float_set_default,/* value_set_default */
        param_spec_rgb_float_validate,   /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_RGB_FLOAT;
    type = g_param_type_register_static ("GeglParamRgbFloat", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_RGB_FLOAT);
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecImageData), /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        NULL,                            /* value_set_default */
        param_spec_image_data_validate,  /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_IMAGE_DATA;
    type = g_param_type_register_static ("GeglParamImageData", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_IMAGE_DATA);
  }
}

GParamSpec*
gegl_param_spec_uint8 (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       guint8 minimum,
                       guint8 maximum,
                       guint8 default_value,
                       GParamFlags  flags)
{
  GeglParamSpecUInt8 *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_UINT8, 
                                 name, nick, blurb, flags);

  spec->minimum = minimum;
  spec->maximum = maximum;
  spec->default_value = default_value;

  return G_PARAM_SPEC (spec);
}

GParamSpec*
gegl_param_spec_float (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gfloat minimum,
                       gfloat maximum,
                       gfloat default_value,
                       GParamFlags  flags)
{
  GeglParamSpecFloat *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_FLOAT, 
                                name, nick, blurb, flags);

  spec->minimum = minimum;
  spec->maximum = maximum;
  spec->default_value = default_value;

  return G_PARAM_SPEC (spec);
}

GParamSpec*  
gegl_param_spec_rgb_float   (const gchar     *name,
                             const gchar     *nick,
                             const gchar     *blurb,
                             gfloat min_red, 
                             gfloat max_red,
                             gfloat min_green, 
                             gfloat max_green,
                             gfloat min_blue, 
                             gfloat max_blue,
                             gfloat default_red,
                             gfloat default_green,
                             gfloat default_blue,
                             GParamFlags     flags)
{
  GeglParamSpecRgbFloat *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_RGB_FLOAT, 
                                name, nick, blurb, flags);

  spec->min_red = min_red;
  spec->max_red = max_red;
  spec->min_green = min_green;
  spec->max_green = max_green;
  spec->min_blue = min_blue;
  spec->max_blue = max_blue;

  spec->default_red = default_red;
  spec->default_green = default_green;
  spec->default_blue = default_blue;

  return G_PARAM_SPEC (spec);
}

GParamSpec*  
gegl_param_spec_rgb_uint8   (const gchar     *name,
                             const gchar     *nick,
                             const gchar     *blurb,
                             guint8 min_red, 
                             guint8 max_red,
                             guint8 min_green, 
                             guint8 max_green,
                             guint8 min_blue, 
                             guint8 max_blue,
                             guint8 default_red,
                             guint8 default_green,
                             guint8 default_blue,
                             GParamFlags     flags)
{
  GeglParamSpecRgbUInt8 *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_RGB_UINT8, 
                                name, nick, blurb, flags);

  spec->min_red = min_red;
  spec->max_red = max_red;
  spec->min_green = min_green;
  spec->max_green = max_green;
  spec->min_blue = min_blue;
  spec->max_blue = max_blue;

  spec->default_red = default_red;
  spec->default_green = default_green;
  spec->default_blue = default_blue;

  return G_PARAM_SPEC (spec);
}

GParamSpec*
gegl_param_spec_image_data (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            GeglRect *rect,
                            GeglColorModel *color_model,
                            GParamFlags  flags)
{
  GeglParamSpecImageData *ispec;

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_IMAGE_DATA, 
                                 name, nick, blurb, flags);
  
  gegl_rect_copy(&ispec->rect, rect);
  ispec->color_model = color_model;
  
  return G_PARAM_SPEC (ispec);
}
