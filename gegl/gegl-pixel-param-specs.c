#include    "gegl-pixel-param-specs.h"
#include    "gegl-pixel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-channel-space.h"

GType GEGL_TYPE_PARAM_PIXEL = 0;
GType GEGL_TYPE_PARAM_PIXEL_RGB_UINT8 = 0;
GType GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8 = 0;
GType GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT = 0;
GType GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT = 0;

/* --- param spec functions --- */

static void 
param_spec_pixel_rgb_uint8_set_default (GParamSpec *pspec,
                                        GValue     *value)
{
  GeglParamSpecPixelRgbUInt8 *spec = GEGL_PARAM_SPEC_PIXEL_RGB_UINT8 (pspec);
  guint8 *p = (guint8*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
}

static gboolean
param_spec_pixel_rgb_uint8_validate (GParamSpec *pspec,
                                     GValue     *value)
{
  GeglParamSpecPixelRgbUInt8 *spec = GEGL_PARAM_SPEC_PIXEL_RGB_UINT8 (pspec);
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
param_spec_pixel_rgba_uint8_set_default (GParamSpec *pspec,
                                         GValue     *value)
{
  GeglParamSpecPixelRgbaUInt8 *spec = GEGL_PARAM_SPEC_PIXEL_RGBA_UINT8 (pspec);
  guint8 *p = (guint8*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
  p[3] = spec->default_alpha;
}

static gboolean
param_spec_pixel_rgba_uint8_validate (GParamSpec *pspec,
                                      GValue     *value)
{
  GeglParamSpecPixelRgbaUInt8 *spec = GEGL_PARAM_SPEC_PIXEL_RGBA_UINT8 (pspec);
  guint8 *p = (guint8*)value->data[1].v_pointer;
  guint8 old_red = p[0];
  guint8 old_green = p[1];
  guint8 old_blue = p[2];
  guint8 old_alpha = p[3];
  
  p[0] = CLAMP (p[0], spec->min_red, spec->max_red);
  p[1] = CLAMP (p[1], spec->min_green, spec->max_green);
  p[2] = CLAMP (p[2], spec->min_blue, spec->max_blue);
  p[3] = CLAMP (p[3], spec->min_alpha, spec->max_alpha);

  return (old_red != p[0]) || 
         (old_green != p[1]) || 
         (old_blue != p[2]) || 
         (old_alpha != p[3]);  
}

static void 
param_spec_pixel_rgb_float_set_default (GParamSpec *pspec,
                                        GValue     *value)
{
  GeglParamSpecPixelRgbFloat *spec = GEGL_PARAM_SPEC_PIXEL_RGB_FLOAT (pspec);
  gfloat *p = (gfloat*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
}

static gboolean
param_spec_pixel_rgb_float_validate (GParamSpec *pspec,
                                     GValue     *value)
{
  GeglParamSpecPixelRgbFloat *spec = GEGL_PARAM_SPEC_PIXEL_RGB_FLOAT (pspec);
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

static void 
param_spec_pixel_rgba_float_set_default (GParamSpec *pspec,
                                         GValue     *value)
{
  GeglParamSpecPixelRgbaFloat *spec = GEGL_PARAM_SPEC_PIXEL_RGBA_FLOAT (pspec);
  gfloat *p = (gfloat*)value->data[1].v_pointer;

  p[0] = spec->default_red;
  p[1] = spec->default_green;
  p[2] = spec->default_blue;
  p[3] = spec->default_alpha;
}

static gboolean
param_spec_pixel_rgba_float_validate (GParamSpec *pspec,
                                      GValue     *value)
{
  GeglParamSpecPixelRgbaFloat *spec = GEGL_PARAM_SPEC_PIXEL_RGBA_FLOAT (pspec);
  gfloat *p = (gfloat*)value->data[1].v_pointer;
  gfloat old_red = p[0];
  gfloat old_green = p[1];
  gfloat old_blue = p[2];
  gfloat old_alpha = p[3];
  
  p[0] = CLAMP (p[0], spec->min_red, spec->max_red);
  p[1] = CLAMP (p[1], spec->min_green, spec->max_green);
  p[2] = CLAMP (p[2], spec->min_blue, spec->max_blue);
  p[3] = CLAMP (p[3], spec->min_alpha, spec->max_alpha);

  return (old_red != p[0]) || 
         (old_green != p[1]) || 
         (old_blue != p[2]) || 
         (old_alpha != p[3]);  
}

static gboolean
param_spec_pixel_validate (GParamSpec *pspec,
                           GValue     *value)
{
#if 0
  GeglParamSpecPixel *spec = GEGL_PARAM_SPEC_PIXEL (pspec);
  gchar *property_name = g_strconcat(pspec->name, 
                                     "-",
                                     gegl_color_model_name(spec->color_model), 
                                     NULL);
  GParamSpec *derived_param_spec = 
    g_object_class_find_property(spec->owner_class, property_name);

  return g_param_value_validate(derived_param_spec, value); 
#endif
  return FALSE;
}

void
gegl_pixel_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecPixel),          /* instance_size */
        16,                                   /* n_preallocs */
        NULL,                                 /* instance_init */
        0x0,                                  /* value_type */
        NULL,                                 /* finalize */
        NULL,                                 /* value_set_default */
        param_spec_pixel_validate,            /* value_validate */
        NULL,                                 /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_PIXEL;
    GEGL_TYPE_PARAM_PIXEL = 
      g_param_type_register_static ("GeglParamPixel", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_PIXEL == 
              g_type_from_name("GeglParamPixel"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecPixelRgbUInt8),   /* instance_size */
        16,                                    /* n_preallocs */
        NULL,                                  /* instance_init */
        0x0,                                   /* value_type */
        NULL,                                  /* finalize */
        param_spec_pixel_rgb_uint8_set_default,/* value_set_default */
        param_spec_pixel_rgb_uint8_validate,   /* value_validate */
        NULL,                                  /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_PIXEL_RGB_UINT8;
    GEGL_TYPE_PARAM_PIXEL_RGB_UINT8 = 
      g_param_type_register_static ("GeglParamPixelRgbUInt8", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_PIXEL_RGB_UINT8 == 
              g_type_from_name("GeglParamPixelRgbUInt8"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecPixelRgbaUInt8),   /* instance_size */
        16,                                    /* n_preallocs */
        NULL,                                  /* instance_init */
        0x0,                                   /* value_type */
        NULL,                                  /* finalize */
        param_spec_pixel_rgba_uint8_set_default,/* value_set_default */
        param_spec_pixel_rgba_uint8_validate,   /* value_validate */
        NULL,                                  /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_PIXEL_RGBA_UINT8;
    GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8 = 
      g_param_type_register_static ("GeglParamPixelRgbaUInt8", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8 == 
              g_type_from_name("GeglParamPixelRgbaUInt8"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecPixelRgbFloat),   /* instance_size */
        16,                                    /* n_preallocs */
        NULL,                                  /* instance_init */
        0x0,                                   /* value_type */
        NULL,                                  /* finalize */
        param_spec_pixel_rgb_float_set_default,/* value_set_default */
        param_spec_pixel_rgb_float_validate,   /* value_validate */
        NULL,                                  /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_PIXEL_RGB_FLOAT;
    GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT = 
       g_param_type_register_static ("GeglParamPixelRgbFloat", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT == 
              g_type_from_name("GeglParamPixelRgbFloat"));
  }

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecPixelRgbaFloat),   /* instance_size */
        16,                                     /* n_preallocs */
        NULL,                                   /* instance_init */
        0x0,                                    /* value_type */
        NULL,                                   /* finalize */
        param_spec_pixel_rgba_float_set_default,/* value_set_default */
        param_spec_pixel_rgba_float_validate,   /* value_validate */
        NULL,                                   /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_PIXEL_RGBA_FLOAT;
    GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT = 
       g_param_type_register_static ("GeglParamPixelRgbaFloat", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT == 
              g_type_from_name("GeglParamPixelRgbaFloat"));
  }
}

GParamSpec*
gegl_param_spec_pixel (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GeglParamSpecPixel *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_PIXEL, 
                                 name, nick, blurb, flags);

  return G_PARAM_SPEC (spec);
}

GParamSpec*  
gegl_param_spec_pixel_rgb_float   (const gchar     *name,
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
  GeglParamSpecPixelRgbFloat *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT, 
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
gegl_param_spec_pixel_rgba_float   (const gchar     *name,
                                    const gchar     *nick,
                                    const gchar     *blurb,
                                    gfloat min_red, 
                                    gfloat max_red,
                                    gfloat min_green, 
                                    gfloat max_green,
                                    gfloat min_blue, 
                                    gfloat max_blue,
                                    gfloat min_alpha, 
                                    gfloat max_alpha,
                                    gfloat default_red,
                                    gfloat default_green,
                                    gfloat default_blue,
                                    gfloat default_alpha,
                                    GParamFlags     flags)
{
  GeglParamSpecPixelRgbaFloat *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT, 
                                name, nick, blurb, flags);

  spec->min_red = min_red;
  spec->max_red = max_red;
  spec->min_green = min_green;
  spec->max_green = max_green;
  spec->min_blue = min_blue;
  spec->max_blue = max_blue;
  spec->min_alpha = min_alpha;
  spec->max_alpha = max_alpha;

  spec->default_red = default_red;
  spec->default_green = default_green;
  spec->default_blue = default_blue;
  spec->default_alpha = default_alpha;

  return G_PARAM_SPEC (spec);
}

GParamSpec*  
gegl_param_spec_pixel_rgb_uint8   (const gchar     *name,
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
  GeglParamSpecPixelRgbUInt8 *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_PIXEL_RGB_UINT8, 
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
gegl_param_spec_pixel_rgba_uint8   (const gchar     *name,
                                    const gchar     *nick,
                                    const gchar     *blurb,
                                    guint8 min_red, 
                                    guint8 max_red,
                                    guint8 min_green, 
                                    guint8 max_green,
                                    guint8 min_blue, 
                                    guint8 max_blue,
                                    guint8 min_alpha, 
                                    guint8 max_alpha,
                                    guint8 default_red,
                                    guint8 default_green,
                                    guint8 default_blue,
                                    guint8 default_alpha,
                                    GParamFlags     flags)
{
  GeglParamSpecPixelRgbaUInt8 *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8, 
                                name, nick, blurb, flags);

  spec->min_red = min_red;
  spec->max_red = max_red;
  spec->min_green = min_green;
  spec->max_green = max_green;
  spec->min_blue = min_blue;
  spec->max_blue = max_blue;
  spec->min_alpha = min_alpha;
  spec->max_alpha = max_alpha;

  spec->default_red = default_red;
  spec->default_green = default_green;
  spec->default_blue = default_blue;
  spec->default_alpha = default_alpha;

  return G_PARAM_SPEC (spec);
}
