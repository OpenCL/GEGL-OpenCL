#include    <gobject/gvaluecollector.h>

#include    "gegl-value-types.h"
#include    "gegl-param-specs.h"
#include    "gegl-types.h"
#include    "gegl-utils.h"
#include    "gegl-data-space.h"
#include    "gegl-color-space.h"
#include    "gegl-color-model.h"
#include    <string.h>

/* --- channel types --- */
GType GEGL_TYPE_CHANNEL = 0;
GType GEGL_TYPE_FLOAT = 0;
GType GEGL_TYPE_UINT8 = 0;

/* --- channel value functions --- */
static void
value_init_uint8 (GValue *value)
{
  value->data[0].v_pointer = gegl_data_space_instance("u8");
  value->data[1].v_uint = 0; 
}

static void
value_free_uint8 (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_copy_uint8 (const GValue *src_value,
                  GValue       *dest_value)
{
  memcpy(dest_value, src_value, sizeof(GValue));
}

static gchar*
value_collect_uint8 (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  value->data[0].v_pointer = gegl_data_space_instance("u8");
  value->data[1].v_uint = collect_values[0].v_int;
  return NULL;
}

static gchar*
value_lcopy_uint8 (const GValue *value,
                   guint         n_collect_values,
                   GTypeCValue  *collect_values,
                   guint         collect_flags)
{
  guint8 *uint8_p = collect_values[0].v_pointer;
  
  if (!uint8_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *uint8_p = value->data[1].v_uint;
  
  return NULL;
}

static void
value_init_float (GValue *value)
{
  value->data[0].v_pointer = gegl_data_space_instance("float");
  value->data[1].v_float = 0.0; 
}

static void
value_free_float (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_copy_float (const GValue *src_value,
                  GValue       *dest_value)
{
  memcpy(dest_value, src_value, sizeof(GValue));
}

static gchar*
value_collect_float (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  value->data[0].v_pointer = gegl_data_space_instance("float");
  value->data[1].v_float = collect_values[0].v_double;
  return NULL;
}

static gchar*
value_lcopy_float (const GValue *value,
                   guint         n_collect_values,
                   GTypeCValue  *collect_values,
                   guint         collect_flags)
{
  gfloat *float_p = collect_values[0].v_pointer;
  
  if (!float_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                            G_VALUE_TYPE_NAME (value));
  
  *float_p = value->data[1].v_float;
  
  return NULL;
}

static void 
gegl_fundamental_types(void)
{
  GTypeInfo info = 
  {
    0,				/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
    NULL,			/* value_table */
  };
  const GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE, };
  GType type;

  g_assert (GEGL_TYPE_CHANNEL == 0);
  GEGL_TYPE_CHANNEL = G_TYPE_MAKE_FUNDAMENTAL(49);
  type = g_type_register_fundamental (GEGL_TYPE_CHANNEL, 
                                      "GeglChannel", 
                                      &info, 
                                      &finfo, 
                                      G_TYPE_FLAG_ABSTRACT);

  g_assert (type == GEGL_TYPE_CHANNEL);

  g_assert (GEGL_TYPE_PIXEL == 0);
  GEGL_TYPE_PIXEL = G_TYPE_MAKE_FUNDAMENTAL(50);
  type = g_type_register_fundamental (GEGL_TYPE_PIXEL, 
                                      "GeglPixel", 
                                      &info, 
                                      &finfo, 
                                      G_TYPE_FLAG_ABSTRACT);

  g_assert (type == GEGL_TYPE_PIXEL);
}

static void
gegl_channel_value_types_init (void)
{
  GTypeInfo info = 
  {
    0,				/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
    NULL,			/* value_table */
  };

  {
    static const GTypeValueTable value_table = 
      {
        value_init_uint8,            /* value_init */
        value_free_uint8,            /* value_free */
        value_copy_uint8,            /* value_copy */
        NULL,			             /* value_peek_pointer */
        "i",                         /* collect_format */
        value_collect_uint8,         /* collect_value */
        "p",                         /* lcopy_format */
        value_lcopy_uint8,           /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_UINT8 == 0);
  GEGL_TYPE_UINT8 = g_type_register_static (GEGL_TYPE_CHANNEL, 
                                            "GeglUInt8", 
                                            &info, 
                                            0);
  g_assert (GEGL_TYPE_UINT8 == g_type_from_name("GeglUInt8"));

  {
    static const GTypeValueTable value_table = 
      {
        value_init_float,            /* value_init */
        value_free_float,            /* value_free */
        value_copy_float,            /* value_copy */
        NULL,			             /* value_peek_pointer */
        "d",	                     /* collect_format */
        value_collect_float,         /* collect_value */
        "p",  			             /* lcopy_format */
        value_lcopy_float,           /* lcopy_value */
      };

    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_FLOAT == 0);
  GEGL_TYPE_FLOAT = g_type_register_static (GEGL_TYPE_CHANNEL, 
                                            "GeglFloat", 
                                            &info, 
                                            0);
  g_assert (GEGL_TYPE_FLOAT == g_type_from_name("GeglFloat"));
}

static void 
value_transform_channel(const GValue *src_value,
                        GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    GeglDataSpace *src_data_space = src_value->data[0].v_pointer;
    GeglDataSpace *dest_data_space = dest_value->data[0].v_pointer;
    GValue  float_value = {0,}; 
    g_value_init(&float_value, GEGL_TYPE_FLOAT);
    gegl_data_space_convert_value_to_float(src_data_space, 
                                           &float_value, 
                                           (GValue*)src_value); 

    gegl_data_space_convert_value_from_float(dest_data_space, 
                                             dest_value, 
                                             &float_value);
  }
}

static void
gegl_channel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_UINT8, GEGL_TYPE_FLOAT, value_transform_channel);
  g_value_register_transform_func(GEGL_TYPE_FLOAT, GEGL_TYPE_UINT8, value_transform_channel);
}

/* --- Channel GValue functions --- */

void
g_value_set_gegl_uint8 (GValue *value,
                        guint8    v_uint8)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_data_space_instance("u8"));

  value->data[1].v_uint = v_uint8;
}

guint8
g_value_get_gegl_uint8 (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_GEGL_UINT8 (value), 0);
  g_return_val_if_fail (value->data[0].v_pointer == 
                        gegl_data_space_instance("u8"), 0);

  return (guint8)value->data[1].v_uint;
}

void
g_value_set_gegl_float (GValue *value,
                        gfloat  v_float)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_data_space_instance("float"));

  value->data[1].v_float = v_float;
}

gfloat
g_value_get_gegl_float (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_GEGL_FLOAT (value), 0);
  g_return_val_if_fail (value->data[0].v_pointer == 
                        gegl_data_space_instance("float"), 0);

  return value->data[1].v_float;
}

/* --- pixel types --- */
GType GEGL_TYPE_PIXEL = 0;
/* GType GEGL_TYPE_RGBA_FLOAT = 0; */
GType GEGL_TYPE_RGB_FLOAT = 0;
/* GType GEGL_TYPE_RGBA_UINT8 = 0; */
GType GEGL_TYPE_RGB_UINT8 = 0;


/* --- pixel value functions --- */
static void
value_init_rgb_float (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgb-float");
  value->data[1].v_pointer = g_new(gfloat, 3); 
}

static void
value_free_rgb_float (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_rgb_float (const GValue *src_value,
                      GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         3 * sizeof(gfloat));
}

static gchar*
value_collect_rgb_float (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  gfloat *float_p = g_new(float, 3); 

  value->data[0].v_pointer = gegl_color_model_instance("rgb-float");

  float_p[0] = collect_values[0].v_double;
  float_p[1] = collect_values[1].v_double;
  float_p[2] = collect_values[2].v_double;
  value->data[1].v_pointer = float_p;

  return NULL;
}

static gchar*
value_lcopy_rgb_float (const GValue *value,
                   guint         n_collect_values,
                   GTypeCValue  *collect_values,
                   guint         collect_flags)
{
  gfloat *r_p = collect_values[0].v_pointer;
  gfloat *g_p = collect_values[1].v_pointer;
  gfloat *b_p = collect_values[2].v_pointer;

  gfloat *float_p = value->data[1].v_pointer;
  
  if (!r_p || !g_p || !b_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *r_p = float_p[0];
  *g_p = float_p[1];
  *b_p = float_p[2];
  
  return NULL;
}

static void
value_init_rgb_uint8 (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgb-u8");
  value->data[1].v_pointer = g_new(guint8, 3); 
}

static void
value_free_rgb_uint8 (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_rgb_uint8 (const GValue *src_value,
                      GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         3 * sizeof(guint8));
}

static gchar*
value_collect_rgb_uint8 (GValue      *value,
                         guint        n_collect_values,
                         GTypeCValue *collect_values,
                         guint        collect_flags)
{
  guint8 *uint8_p = g_new(guint8, 3); 

  value->data[0].v_pointer = gegl_color_model_instance("rgb-u8");

  uint8_p[0] = collect_values[0].v_int;
  uint8_p[1] = collect_values[1].v_int;
  uint8_p[2] = collect_values[2].v_int;
  value->data[1].v_pointer = uint8_p;

  return NULL;
}

static gchar*
value_lcopy_rgb_uint8 (const GValue *value,
                       guint         n_collect_values,
                       GTypeCValue  *collect_values,
                       guint         collect_flags)
{
  guint8 *r_p = collect_values[0].v_pointer;
  guint8 *g_p = collect_values[1].v_pointer;
  guint8 *b_p = collect_values[2].v_pointer;

  guint8 *uint8_p = value->data[1].v_pointer;
  
  if (!r_p || !g_p || !b_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *r_p = uint8_p[0];
  *g_p = uint8_p[1];
  *b_p = uint8_p[2];
  
  return NULL;
}


static void
gegl_pixel_value_types_init (void)
{
  GTypeInfo info = 
  {
    0,				/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
    NULL,			/* value_table */
  };

  {
    static const GTypeValueTable value_table = 
      {
        value_init_rgb_float,        /* value_init */
        value_free_rgb_float,        /* value_free */
        value_copy_rgb_float,        /* value_copy */
        NULL,			             /* value_peek_pointer */
        "ddd",                       /* collect_format */
        value_collect_rgb_float,     /* collect_value */
        "ppp",                       /* lcopy_format */
        value_lcopy_rgb_float,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_RGB_FLOAT == 0);
  GEGL_TYPE_RGB_FLOAT = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglRgbFloat", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_RGB_FLOAT == g_type_from_name("GeglRgbFloat"));

  {
    static const GTypeValueTable value_table = 
      {
        value_init_rgb_uint8,           /* value_init */
        value_free_rgb_uint8,           /* value_free */
        value_copy_rgb_uint8,           /* value_copy */
        NULL,			                /* value_peek_pointer */
        "iii",                          /* collect_format */
        value_collect_rgb_uint8,        /* collect_value */
        "ppp",                          /* lcopy_format */
        value_lcopy_rgb_uint8,          /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_RGB_UINT8 == 0);
  GEGL_TYPE_RGB_UINT8 = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglRgbUInt8", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_RGB_UINT8 == g_type_from_name("GeglRgbUInt8"));
}

static void 
value_transform_pixel(const GValue *src_value,
                      GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    GeglDataSpace *float_data_space = gegl_data_space_instance("float");

    GeglColorModel *s_cm = src_value->data[0].v_pointer;
    GeglColorModel *d_cm = dest_value->data[0].v_pointer;

    GeglColorSpace *src_color_space = gegl_color_model_color_space(s_cm);
    GeglColorSpace *dest_color_space = gegl_color_model_color_space(d_cm);

    GeglDataSpace *src_data_space = gegl_color_model_data_space(s_cm);
    GeglDataSpace *dest_data_space = gegl_color_model_data_space(d_cm);

    gint src_num_colors = gegl_color_model_num_colors(s_cm);
    gint dest_num_colors = gegl_color_model_num_colors(d_cm);

#if 0
    gboolean src_has_alpha = gegl_color_model_has_alpha(s_cm);
    gboolean dest_has_alpha = gegl_color_model_has_alpha(s_cm);

    gint *src_bits = gegl_color_model_bits_per_channel(s_cm);
    gint *dest_bits = gegl_color_model_bits_per_channel(d_cm);

    gint src_alpha_index = gegl_color_model_alpha_channel(s_cm);
    gint dest_alpha_index = gegl_color_model_alpha_channel(d_cm);

    gpointer src_alpha = NULL;
    gpointer dest_alpha = NULL;
    guint8 * src_uint8_ptr;
    guint8 * dest_uint8_ptr;
#endif

    gpointer src_data = src_value->data[1].v_pointer;
    gpointer dest_data = dest_value->data[1].v_pointer;

    gfloat * src_float_data = NULL;
    gfloat * dest_float_data = NULL;
    gfloat * xyz_data = NULL;

#if 0
    if(dest_has_alpha)
      {
        gint i;
        dest_uint8_ptr = (guint8*)dest_data;

        for(i = 0; i < dest_alpha_index; i++)
          dest_uint8_ptr += (dest_bits[i]/8);  

        dest_alpha = dest_uint8_ptr;

        if(src_has_alpha)
          {
            gint i;
            src_uint8_ptr = (guint8*)src_data;

            for(i = 0; i < src_alpha_index; i++)
              src_uint8_ptr += (src_bits[i]/8);  

            src_alpha = src_uint8_ptr;
          }
        else
          {
            gfloat alpha_float_one = 1.0;
            if(
            gegl_data_space_convert_from_float(dest_data_space,
          }
      }
#endif

    if(src_data_space != float_data_space)
      {
        src_float_data = g_new(float, src_num_colors);
        gegl_data_space_convert_to_float(src_data_space, 
                                         src_float_data, 
                                         src_data, 
                                         src_num_colors); 
      }
    else
      src_float_data = (gfloat*)src_data;

    if(src_color_space != dest_color_space) 
      {
        dest_float_data = g_new(float, dest_num_colors);
        xyz_data = g_new(float, 3);

        gegl_color_space_convert_to_xyz(src_color_space, 
                                        xyz_data, 
                                        src_float_data, 
                                        1); 

        gegl_color_space_convert_from_xyz(dest_color_space, 
                                          dest_float_data, 
                                          xyz_data, 
                                          1); 
      }
    else 
      dest_float_data = src_float_data;

    if(dest_data_space != float_data_space)
      gegl_data_space_convert_from_float(dest_data_space, 
                                         dest_data, 
                                         dest_float_data, 
                                         dest_num_colors);
    else
      memcpy(dest_data, 
             dest_float_data, 
             dest_num_colors * sizeof(gfloat));

    if(src_data_space != float_data_space)
      g_free(src_float_data);

    if(src_color_space != dest_color_space)
      {
        g_free(dest_float_data);
        g_free(xyz_data);
      }
  }
}

/* --- Pixel GValue functions --- */

static void
gegl_pixel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_RGB_FLOAT, GEGL_TYPE_RGB_UINT8, value_transform_pixel);
  g_value_register_transform_func(GEGL_TYPE_RGB_UINT8, GEGL_TYPE_RGB_FLOAT, value_transform_pixel);
}

void
g_value_set_gegl_rgb_float (GValue *value,
                            gfloat  red,
                            gfloat  green,
                            gfloat  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_RGB_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-float"));

  {
    gfloat *float_p = value->data[1].v_pointer;

    float_p[0] = red;
    float_p[1] = green;
    float_p[2] = blue;
  }
}

void
g_value_get_gegl_rgb_float (const GValue *value,
                            gfloat *red,
                            gfloat *green,
                            gfloat *blue)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_RGB_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-float"));
  {
    gfloat *float_p = value->data[1].v_pointer;

    *red =   float_p[0]; 
    *green = float_p[1]; 
    *blue =  float_p[2]; 
  }
}

void
g_value_set_gegl_rgb_uint8 (GValue *value,
                            guint8  red,
                            guint8  green,
                            guint8  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_RGB_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-u8"));

  {
    guint8 *uint8_p = value->data[1].v_pointer;

    uint8_p[0] = red;
    uint8_p[1] = green;
    uint8_p[2] = blue;
  }
}

void
g_value_get_gegl_rgb_uint8 (const GValue *value,
                            guint8 *red,
                            guint8 *green,
                            guint8 *blue)
{
  g_return_if_fail (G_VALUE_HOLDS_GEGL_RGB_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-u8"));
  {
    guint8 *uint8_p = value->data[1].v_pointer;

    *red =   uint8_p[0]; 
    *green = uint8_p[1]; 
    *blue =  uint8_p[2]; 
  }
}

gpointer
g_value_pixel_get_data(GValue * value)
{
  g_return_val_if_fail (g_type_is_a(G_VALUE_TYPE (value), GEGL_TYPE_PIXEL), NULL);
  g_return_val_if_fail (value->data[0].v_pointer, NULL);

  return value->data[1].v_pointer;
}

GeglColorModel *
g_value_pixel_get_color_model(GValue * value)
{
  g_return_val_if_fail (g_type_is_a(G_VALUE_TYPE (value), GEGL_TYPE_PIXEL), NULL);
  g_return_val_if_fail (value->data[0].v_pointer, NULL);

  return value->data[0].v_pointer;
}

void
gegl_value_types_init (void)
{
  gegl_fundamental_types();
  gegl_channel_value_types_init();
  gegl_pixel_value_types_init();
}

void
gegl_value_transform_init(void) 
{
  gegl_channel_value_transform_init();
  gegl_pixel_value_transform_init();
}

void
g_value_channel_print(GValue * value)
{
  GeglDataSpace * data_space = value->data[0].v_pointer;
  gchar * name = "none"; 

  if(data_space)
    name = gegl_data_space_name(data_space);

  LOG_DIRECT("data space name is %s", name);

  if(data_space == gegl_data_space_instance("u8"))
    LOG_DIRECT("value is %d", value->data[1].v_uint);
  else if(data_space == gegl_data_space_instance("float"))
    LOG_DIRECT("value is %f", value->data[1].v_float);
  else if(data_space == gegl_data_space_instance("u16"))
    LOG_DIRECT("value is %d", value->data[1].v_uint);
}
