#include    <gobject/gvaluecollector.h>

#include    "gegl-pixel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-channel-space.h"
#include    "gegl-color-space.h"
#include    "gegl-color-model.h"
#include    <string.h>

/* --- pixel types --- */
GType GEGL_TYPE_PIXEL_RGB_FLOAT = 0;
GType GEGL_TYPE_PIXEL_RGBA_FLOAT = 0;
GType GEGL_TYPE_PIXEL_RGB_UINT8 = 0;
GType GEGL_TYPE_PIXEL_RGBA_UINT8 = 0;

/* --- pixel value functions --- */
static void
value_init_pixel_rgb_float (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgb-float");
  value->data[1].v_pointer = g_new(gfloat, 3); 
}

static void
value_free_pixel_rgb_float (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_rgb_float (const GValue *src_value,
                      GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         3 * sizeof(gfloat));
}

static gchar*
value_collect_pixel_rgb_float (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  gfloat *float_p = g_new(gfloat, 3); 

  value->data[0].v_pointer = gegl_color_model_instance("rgb-float");

  float_p[0] = collect_values[0].v_double;
  float_p[1] = collect_values[1].v_double;
  float_p[2] = collect_values[2].v_double;
  value->data[1].v_pointer = float_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_rgb_float (const GValue *value,
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
value_init_pixel_rgba_float (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgba-float");
  value->data[1].v_pointer = g_new(gfloat, 4); 
}

static void
value_free_pixel_rgba_float (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_rgba_float (const GValue *src_value,
                             GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         4 * sizeof(gfloat));
}

static gchar*
value_collect_pixel_rgba_float (GValue      *value,
                                guint        n_collect_values,
                                GTypeCValue *collect_values,
                                guint        collect_flags)
{
  gfloat *float_p = g_new(gfloat, 4); 

  value->data[0].v_pointer = gegl_color_model_instance("rgba-float");

  float_p[0] = collect_values[0].v_double;
  float_p[1] = collect_values[1].v_double;
  float_p[2] = collect_values[2].v_double;
  float_p[3] = collect_values[3].v_double;

  value->data[1].v_pointer = float_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_rgba_float (const GValue *value,
                              guint         n_collect_values,
                              GTypeCValue  *collect_values,
                              guint         collect_flags)
{
  gfloat *r_p = collect_values[0].v_pointer;
  gfloat *g_p = collect_values[1].v_pointer;
  gfloat *b_p = collect_values[2].v_pointer;
  gfloat *a_p = collect_values[3].v_pointer;

  gfloat *float_p = value->data[1].v_pointer;
  
  if (!r_p || !g_p || !b_p || !a_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *r_p = float_p[0];
  *g_p = float_p[1];
  *b_p = float_p[2];
  *a_p = float_p[3];
  
  return NULL;
}

static void
value_init_pixel_rgb_uint8 (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgb-uint8");
  value->data[1].v_pointer = g_new(guint8, 3); 
}

static void
value_free_pixel_rgb_uint8 (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_rgb_uint8 (const GValue *src_value,
                      GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         3 * sizeof(guint8));
}

static gchar*
value_collect_pixel_rgb_uint8 (GValue      *value,
                         guint        n_collect_values,
                         GTypeCValue *collect_values,
                         guint        collect_flags)
{
  guint8 *uint8_p = g_new(guint8, 3); 

  value->data[0].v_pointer = gegl_color_model_instance("rgb-uint8");

  uint8_p[0] = collect_values[0].v_int;
  uint8_p[1] = collect_values[1].v_int;
  uint8_p[2] = collect_values[2].v_int;
  value->data[1].v_pointer = uint8_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_rgb_uint8 (const GValue *value,
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
value_init_pixel_rgba_uint8 (GValue *value)
{
  value->data[0].v_pointer = gegl_color_model_instance("rgba-uint8");
  value->data[1].v_pointer = g_new(guint8, 4); 
}

static void
value_free_pixel_rgba_uint8 (GValue *value)
{
  value->data[0].v_pointer = NULL;
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_rgba_uint8 (const GValue *src_value,
                             GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         4 * sizeof(guint8));
}

static gchar*
value_collect_pixel_rgba_uint8 (GValue      *value,
                                guint        n_collect_values,
                                GTypeCValue *collect_values,
                                guint        collect_flags)
{
  guint8 *uint8_p = g_new(guint8, 4); 

  value->data[0].v_pointer = gegl_color_model_instance("rgba-uint8");

  uint8_p[0] = collect_values[0].v_int;
  uint8_p[1] = collect_values[1].v_int;
  uint8_p[2] = collect_values[2].v_int;
  uint8_p[3] = collect_values[3].v_int;

  value->data[1].v_pointer = uint8_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_rgba_uint8 (const GValue *value,
                              guint         n_collect_values,
                              GTypeCValue  *collect_values,
                              guint         collect_flags)
{
  guint8 *r_p = collect_values[0].v_pointer;
  guint8 *g_p = collect_values[1].v_pointer;
  guint8 *b_p = collect_values[2].v_pointer;
  guint8 *a_p = collect_values[3].v_pointer;

  guint8 *uint8_p = value->data[1].v_pointer;
  
  if (!r_p || !g_p || !b_p || !a_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *r_p = uint8_p[0];
  *g_p = uint8_p[1];
  *b_p = uint8_p[2];
  *a_p = uint8_p[3];
  
  return NULL;
}

void
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
        value_init_pixel_rgb_float,        /* value_init */
        value_free_pixel_rgb_float,        /* value_free */
        value_copy_pixel_rgb_float,        /* value_copy */
        NULL,			                   /* value_peek_pointer */
        "ddd",                             /* collect_format */
        value_collect_pixel_rgb_float,     /* collect_value */
        "ppp",                             /* lcopy_format */
        value_lcopy_pixel_rgb_float,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_RGB_FLOAT == 0);
  GEGL_TYPE_PIXEL_RGB_FLOAT = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglPixelRgbFloat", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_PIXEL_RGB_FLOAT == g_type_from_name("GeglPixelRgbFloat"));

  {
    static const GTypeValueTable value_table = 
      {
        value_init_pixel_rgba_float,       /* value_init */
        value_free_pixel_rgba_float,       /* value_free */
        value_copy_pixel_rgba_float,       /* value_copy */
        NULL,			                   /* value_peek_pointer */
        "dddd",                            /* collect_format */
        value_collect_pixel_rgba_float,    /* collect_value */
        "pppp",                            /* lcopy_format */
        value_lcopy_pixel_rgba_float,      /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_RGBA_FLOAT == 0);
  GEGL_TYPE_PIXEL_RGBA_FLOAT = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglPixelRgbaFloat", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_PIXEL_RGBA_FLOAT == g_type_from_name("GeglPixelRgbaFloat"));

  {
    static const GTypeValueTable value_table = 
      {
        value_init_pixel_rgb_uint8,        /* value_init */
        value_free_pixel_rgb_uint8,        /* value_free */
        value_copy_pixel_rgb_uint8,        /* value_copy */
        NULL,			                   /* value_peek_pointer */
        "iii",                             /* collect_format */
        value_collect_pixel_rgb_uint8,     /* collect_value */
        "ppp",                             /* lcopy_format */
        value_lcopy_pixel_rgb_uint8,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_RGB_UINT8 == 0);
  GEGL_TYPE_PIXEL_RGB_UINT8 = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                      "GeglPixelRgbUInt8", 
                                                      &info, 
                                                      0);
  g_assert (GEGL_TYPE_PIXEL_RGB_UINT8 == g_type_from_name("GeglPixelRgbUInt8"));

  {
    static const GTypeValueTable value_table = 
      {
        value_init_pixel_rgba_uint8,        /* value_init */
        value_free_pixel_rgba_uint8,        /* value_free */
        value_copy_pixel_rgba_uint8,        /* value_copy */
        NULL,			                    /* value_peek_pointer */
        "iiii",                             /* collect_format */
        value_collect_pixel_rgba_uint8,     /* collect_value */
        "pppp",                             /* lcopy_format */
        value_lcopy_pixel_rgba_uint8,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_RGBA_UINT8 == 0);
  GEGL_TYPE_PIXEL_RGBA_UINT8 = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                       "GeglPixelRgbaUInt8", 
                                                       &info, 
                                                       0);
  g_assert (GEGL_TYPE_PIXEL_RGBA_UINT8 == g_type_from_name("GeglPixelRgbaUInt8"));
}

static void 
value_transform_pixel(const GValue *src_value,
                      GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    GeglChannelSpace *float_channel_space = gegl_channel_space_instance("float");

    GeglColorModel *s_cm = src_value->data[0].v_pointer;
    GeglColorModel *d_cm = dest_value->data[0].v_pointer;

    GeglColorSpace *src_color_space = gegl_color_model_color_space(s_cm);
    GeglColorSpace *dest_color_space = gegl_color_model_color_space(d_cm);

    GeglChannelSpace *src_channel_space = gegl_color_model_channel_space(s_cm);
    GeglChannelSpace *dest_channel_space = gegl_color_model_channel_space(d_cm);

    gint src_num_colors = gegl_color_model_num_colors(s_cm);
    gint dest_num_colors = gegl_color_model_num_colors(d_cm);

    gint *src_bits = gegl_color_model_bits_per_channel(s_cm);
    gint *dest_bits = gegl_color_model_bits_per_channel(d_cm);

    gboolean src_has_alpha = gegl_color_model_has_alpha(s_cm);
    gboolean dest_has_alpha = gegl_color_model_has_alpha(d_cm);

    gpointer src_data = src_value->data[1].v_pointer;
    gpointer dest_data = dest_value->data[1].v_pointer;

    gfloat * src_float_data = NULL;
    gfloat * dest_float_data = NULL;
    gfloat * xyz_data = NULL;

    if(dest_has_alpha && src_has_alpha)
      {
        gint src_alpha_index = gegl_color_model_alpha_channel(s_cm);
        gpointer src_alpha = NULL;
        guint8 * src_uint8_ptr;
        gfloat src_float_alpha;

        gint dest_alpha_index = gegl_color_model_alpha_channel(d_cm);
        gpointer dest_alpha = NULL;
        guint8 * dest_uint8_ptr;
        gfloat dest_float_alpha = 1.0;

        gint i;

        dest_uint8_ptr = (guint8*)dest_data;
        for(i = 0; i < dest_alpha_index; i++)
          dest_uint8_ptr += (dest_bits[i]/8);  
        dest_alpha = dest_uint8_ptr;

        src_uint8_ptr = (guint8*)src_data;
        for(i = 0; i < src_alpha_index; i++)
          src_uint8_ptr += (src_bits[i]/8);  
        src_alpha = src_uint8_ptr;

        gegl_channel_space_convert_to_float(src_channel_space,
                                         &src_float_alpha,
                                         src_alpha,
                                         1);

        gegl_channel_space_convert_from_float(src_channel_space,
                                           dest_alpha,
                                           &dest_float_alpha,
                                           1);

      }
    else if(dest_has_alpha)
      {
        gint dest_alpha_index = gegl_color_model_alpha_channel(d_cm);
        gpointer dest_alpha = NULL;
        guint8 * dest_uint8_ptr;
        gfloat dest_float_alpha = 1.0;
        gint i;
        gfloat *a;

        dest_uint8_ptr = (guint8*)dest_data;
        for(i = 0; i < dest_alpha_index; i++)
          dest_uint8_ptr += (dest_bits[i]/8);  
        dest_alpha = dest_uint8_ptr;

        gegl_channel_space_convert_from_float(dest_channel_space,
                                           dest_alpha,
                                           &dest_float_alpha,
                                           1);

        a = dest_alpha;
      }

    if(src_channel_space != float_channel_space)
      {
        src_float_data = g_new(float, src_num_colors);
        gegl_channel_space_convert_to_float(src_channel_space, 
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

    if(dest_channel_space != float_channel_space)
      gegl_channel_space_convert_from_float(dest_channel_space, 
                                         dest_data, 
                                         dest_float_data, 
                                         dest_num_colors);
    else
      memcpy(dest_data, 
             dest_float_data, 
             dest_num_colors * sizeof(gfloat));

    if(src_channel_space != float_channel_space)
      g_free(src_float_data);

    if(src_color_space != dest_color_space)
      {
        g_free(dest_float_data);
        g_free(xyz_data);
      }
  }
}

/* --- Pixel GValue functions --- */

void
gegl_pixel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_FLOAT, GEGL_TYPE_PIXEL_RGB_UINT8, value_transform_pixel);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGBA_FLOAT, GEGL_TYPE_PIXEL_RGB_FLOAT, value_transform_pixel);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_FLOAT, GEGL_TYPE_PIXEL_RGBA_FLOAT, value_transform_pixel);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_UINT8, GEGL_TYPE_PIXEL_RGB_FLOAT, value_transform_pixel);
}

void
g_value_set_pixel_rgb_float (GValue *value,
                            gfloat  red,
                            gfloat  green,
                            gfloat  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_FLOAT (value));
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
g_value_get_pixel_rgb_float (const GValue *value,
                             gfloat *red,
                             gfloat *green,
                             gfloat *blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_FLOAT (value));
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
g_value_set_pixel_rgba_float (GValue *value,
                              gfloat  red,
                              gfloat  green,
                              gfloat  blue,
                              gfloat  alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGBA_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgba-float"));

  {
    gfloat *float_p = value->data[1].v_pointer;

    float_p[0] = red;
    float_p[1] = green;
    float_p[2] = blue;
    float_p[3] = alpha;
  }
}

void
g_value_get_pixel_rgba_float (const GValue *value,
                              gfloat *red,
                              gfloat *green,
                              gfloat *blue,
                              gfloat *alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGBA_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgba-float"));
  {
    gfloat *float_p = value->data[1].v_pointer;

    *red =   float_p[0]; 
    *green = float_p[1]; 
    *blue =  float_p[2]; 
    *alpha = float_p[3]; 
  }
}

void
g_value_set_pixel_rgb_uint8 (GValue *value,
                            guint8  red,
                            guint8  green,
                            guint8  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-uint8"));

  {
    guint8 *uint8_p = value->data[1].v_pointer;

    uint8_p[0] = red;
    uint8_p[1] = green;
    uint8_p[2] = blue;
  }
}

void
g_value_get_pixel_rgb_uint8 (const GValue *value,
                            guint8 *red,
                            guint8 *green,
                            guint8 *blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgb-uint8"));
  {
    guint8 *uint8_p = value->data[1].v_pointer;

    *red =   uint8_p[0]; 
    *green = uint8_p[1]; 
    *blue =  uint8_p[2]; 
  }
}

void
g_value_set_pixel_rgba_uint8 (GValue *value,
                              guint8  red,
                              guint8  green,
                              guint8  blue,
                              guint8  alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGBA_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgba-uint8"));

  {
    guint8 *uint8_p = value->data[1].v_pointer;

    uint8_p[0] = red;
    uint8_p[1] = green;
    uint8_p[2] = blue;
    uint8_p[3] = alpha;
  }
}

void
g_value_get_pixel_rgba_uint8 (const GValue *value,
                              guint8 *red,
                              guint8 *green,
                              guint8 *blue,
                              guint8 *alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_color_model_instance("rgba-uint8"));
  {
    guint8 *uint8_p = value->data[1].v_pointer;

    *red =   uint8_p[0]; 
    *green = uint8_p[1]; 
    *blue =  uint8_p[2]; 
    *alpha =  uint8_p[3]; 
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
