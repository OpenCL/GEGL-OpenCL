#include    <gobject/gvaluecollector.h>
#include    "gegl-pixel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-color-space.h"
#include    "gegl-color-model.h"
#include    <string.h>

/* --- float pixel types --- */
GType GEGL_TYPE_PIXEL_RGB_FLOAT = 0;
GType GEGL_TYPE_PIXEL_RGBA_FLOAT = 0;
GType GEGL_TYPE_PIXEL_GRAY_FLOAT = 0;
GType GEGL_TYPE_PIXEL_GRAYA_FLOAT = 0;

/* --- uint8 pixel types --- */
GType GEGL_TYPE_PIXEL_RGB_UINT8 = 0;
GType GEGL_TYPE_PIXEL_RGBA_UINT8 = 0;

/* --- rgb float --- */
static void
value_init_pixel_rgb_float (GValue *value)
{
  value->data[0].v_pointer = g_strdup("rgb-float");
  value->data[1].v_pointer = g_new(gfloat, 3); 
}

static void
value_free_pixel_rgb_float (GValue *value)
{
  g_free(value->data[0].v_pointer);
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

  value->data[0].v_pointer = g_strdup("rgb-float");

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

void
gegl_pixel_value_rgb_float_init(void)
{
  static PixelValueInfo pixel_value_info_rgb_float = 
        {"GeglChannelFloat", "rgb", "rgb-float", FALSE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

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

  g_type_set_qdata(GEGL_TYPE_PIXEL_RGB_FLOAT, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_rgb_float);
}

void
g_value_set_pixel_rgb_float (GValue *value,
                            gfloat  red,
                            gfloat  green,
                            gfloat  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_FLOAT (value));

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
  {
    gfloat *float_p = value->data[1].v_pointer;

    *red =   float_p[0]; 
    *green = float_p[1]; 
    *blue =  float_p[2]; 
  }
}

/* --- rgba float --- */

static void
value_init_pixel_rgba_float (GValue *value)
{
  value->data[0].v_pointer = g_strdup("rgba-float");
  value->data[1].v_pointer = g_new(gfloat, 4); 
}

static void
value_free_pixel_rgba_float (GValue *value)
{
  g_free(value->data[0].v_pointer);
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

  value->data[0].v_pointer = g_strdup("rgba-float");

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

void
gegl_pixel_value_rgba_float_init(void)
{
  static PixelValueInfo pixel_value_info_rgba_float = 
        {"GeglChannelFloat", "rgb", "rgba-float", TRUE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

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

  g_type_set_qdata(GEGL_TYPE_PIXEL_RGBA_FLOAT, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_rgba_float);
}

void
g_value_set_pixel_rgba_float (GValue *value,
                              gfloat  red,
                              gfloat  green,
                              gfloat  blue,
                              gfloat  alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGBA_FLOAT (value));

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
  {
    gfloat *float_p = value->data[1].v_pointer;

    *red =   float_p[0]; 
    *green = float_p[1]; 
    *blue =  float_p[2]; 
    *alpha = float_p[3]; 
  }
}

/* --- rgb uint8 --- */

static void
value_init_pixel_rgb_uint8 (GValue *value)
{
  value->data[0].v_pointer = g_strdup("rgb-uint8");
  value->data[1].v_pointer = g_new(guint8, 3); 
}

static void
value_free_pixel_rgb_uint8 (GValue *value)
{
  g_free(value->data[0].v_pointer);
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

  value->data[0].v_pointer = g_strdup("rgb-uint8");

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

void
gegl_pixel_value_rgb_uint8_init(void)
{
  static PixelValueInfo pixel_value_info_rgb_uint8 = 
        {"GeglChannelUInt8", "rgb", "rgb-uint8", FALSE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

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

  g_type_set_qdata(GEGL_TYPE_PIXEL_RGB_UINT8, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_rgb_uint8);
}

void
g_value_set_pixel_rgb_uint8 (GValue *value,
                            guint8  red,
                            guint8  green,
                            guint8  blue)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGB_UINT8 (value));

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
  {
    guint8 *uint8_p = value->data[1].v_pointer;

    *red =   uint8_p[0]; 
    *green = uint8_p[1]; 
    *blue =  uint8_p[2]; 
  }
}

/* --- rgba uint8 --- */

static void
value_init_pixel_rgba_uint8 (GValue *value)
{
  value->data[0].v_pointer = g_strdup("rgba-uint8");
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

  value->data[0].v_pointer = g_strdup("rgba-uint8");

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
gegl_pixel_value_rgba_uint8_init(void)
{
  static PixelValueInfo pixel_value_info_rgba_uint8 = 
        {"GeglChannelUInt8", "rgb", "rgba-uint8", TRUE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

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

  g_type_set_qdata(GEGL_TYPE_PIXEL_RGBA_UINT8, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_rgba_uint8);
}

void
g_value_set_pixel_rgba_uint8 (GValue *value,
                              guint8  red,
                              guint8  green,
                              guint8  blue,
                              guint8  alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_RGBA_UINT8 (value));

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
  {
    guint8 *uint8_p = value->data[1].v_pointer;

    *red =   uint8_p[0]; 
    *green = uint8_p[1]; 
    *blue =  uint8_p[2]; 
    *alpha =  uint8_p[3]; 
  }
}

/* --- gray float --- */

static void
value_init_pixel_gray_float (GValue *value)
{
  value->data[0].v_pointer = g_strdup("gray-float");
  value->data[1].v_pointer = g_new(gfloat, 1); 
}

static void
value_free_pixel_gray_float (GValue *value)
{
  g_free(value->data[0].v_pointer);
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_gray_float (const GValue *src_value,
                      GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         1 * sizeof(gfloat));
}

static gchar*
value_collect_pixel_gray_float (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  gfloat *float_p = g_new(gfloat, 1); 

  value->data[0].v_pointer = g_strdup("gray-float");

  float_p[0] = collect_values[0].v_double;
  value->data[1].v_pointer = float_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_gray_float (const GValue *value,
                   guint         n_collect_values,
                   GTypeCValue  *collect_values,
                   guint         collect_flags)
{
  gfloat *gray_p = collect_values[0].v_pointer;
  gfloat *float_p = value->data[1].v_pointer;
  
  if (!gray_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *gray_p = float_p[0];
  
  return NULL;
}

void
gegl_pixel_value_gray_float_init(void)
{
  static PixelValueInfo pixel_value_info_gray_float = 
        {"GeglChannelFloat", "gray", "gray-float", FALSE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

  {
    static const GTypeValueTable value_table = 
      {
        value_init_pixel_gray_float,        /* value_init */
        value_free_pixel_gray_float,        /* value_free */
        value_copy_pixel_gray_float,        /* value_copy */
        NULL,			                    /* value_peek_pointer */
        "ddd",                              /* collect_format */
        value_collect_pixel_gray_float,     /* collect_value */
        "ppp",                              /* lcopy_format */
        value_lcopy_pixel_gray_float,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_GRAY_FLOAT == 0);
  GEGL_TYPE_PIXEL_GRAY_FLOAT = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglPixelGrayFloat", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_PIXEL_GRAY_FLOAT == g_type_from_name("GeglPixelGrayFloat"));

  g_type_set_qdata(GEGL_TYPE_PIXEL_GRAY_FLOAT, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_gray_float);
}

void
g_value_set_pixel_gray_float (GValue *value,
                              gfloat  gray)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_GRAY_FLOAT (value));

  {
    gfloat *float_p = value->data[1].v_pointer;

    float_p[0] = gray;
  }
}

void
g_value_get_pixel_gray_float (const GValue *value,
                              gfloat *gray)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_GRAY_FLOAT (value));

  {
    gfloat *float_p = value->data[1].v_pointer;

    *gray =   float_p[0]; 
  }
}

/* --- graya float --- */

static void
value_init_pixel_graya_float (GValue *value)
{
  value->data[0].v_pointer = g_strdup("graya-float");
  value->data[1].v_pointer = g_new(gfloat, 2); 
}

static void
value_free_pixel_graya_float (GValue *value)
{
  g_free(value->data[0].v_pointer);
  g_free(value->data[1].v_pointer); 
}

static void
value_copy_pixel_graya_float (const GValue *src_value,
                             GValue       *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         2 * sizeof(gfloat));
}

static gchar*
value_collect_pixel_graya_float (GValue      *value,
                                guint        n_collect_values,
                                GTypeCValue *collect_values,
                                guint        collect_flags)
{
  gfloat *float_p = g_new(gfloat, 2); 

  value->data[0].v_pointer = g_strdup("graya-float");

  float_p[0] = collect_values[0].v_double;
  float_p[1] = collect_values[1].v_double;

  value->data[1].v_pointer = float_p;

  return NULL;
}

static gchar*
value_lcopy_pixel_graya_float (const GValue *value,
                              guint         n_collect_values,
                              GTypeCValue  *collect_values,
                              guint         collect_flags)
{
  gfloat *gray_p = collect_values[0].v_pointer;
  gfloat *a_p = collect_values[1].v_pointer;

  gfloat *float_p = value->data[1].v_pointer;
  
  if (!gray_p || !a_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *gray_p = float_p[0];
  *a_p = float_p[1];
  
  return NULL;
}

void
gegl_pixel_value_graya_float_init(void)
{
  static PixelValueInfo pixel_value_info_graya_float = 
        {"GeglChannelFloat", "gray", "graya-float", TRUE};

  GTypeInfo info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL,};

  {
    static const GTypeValueTable value_table = 
      {
        value_init_pixel_graya_float,        /* value_init */
        value_free_pixel_graya_float,        /* value_free */
        value_copy_pixel_graya_float,        /* value_copy */
        NULL,			                     /* value_peek_pointer */
        "ddd",                               /* collect_format */
        value_collect_pixel_graya_float,     /* collect_value */
        "ppp",                               /* lcopy_format */
        value_lcopy_pixel_graya_float,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_PIXEL_GRAYA_FLOAT == 0);
  GEGL_TYPE_PIXEL_GRAYA_FLOAT = g_type_register_static (GEGL_TYPE_PIXEL, 
                                                "GeglPixelGrayaFloat", 
                                                &info, 
                                                0);
  g_assert (GEGL_TYPE_PIXEL_GRAYA_FLOAT == g_type_from_name("GeglPixelGrayaFloat"));

  g_type_set_qdata(GEGL_TYPE_PIXEL_GRAYA_FLOAT, 
                   g_quark_from_string("pixel_value_info"), 
                   &pixel_value_info_graya_float);
}

void
g_value_set_pixel_graya_float (GValue *value,
                               gfloat  gray,
                               gfloat  alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_GRAYA_FLOAT (value));

  {
    gfloat *float_p = value->data[1].v_pointer;

    float_p[0] = gray;
    float_p[1] = alpha;
  }
}

void
g_value_get_pixel_graya_float (const GValue *value,
                              gfloat *gray,
                              gfloat *alpha)
{
  g_return_if_fail (G_VALUE_HOLDS_PIXEL_GRAYA_FLOAT (value));
  {
    gfloat *float_p = value->data[1].v_pointer;

    *gray =   float_p[0]; 
    *alpha =  float_p[1]; 
  }
}

void
gegl_pixel_value_types_init (void)
{
  gegl_pixel_value_rgb_float_init();
  gegl_pixel_value_rgba_float_init();
  gegl_pixel_value_rgb_uint8_init();
  gegl_pixel_value_rgba_uint8_init();
  gegl_pixel_value_gray_float_init();
  gegl_pixel_value_graya_float_init();
}


/* --- transform functions --- */

static void 
transform_rgb_float_rgb_uint8(const GValue *src_value,
                              GValue *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  { 
    gint r,g,b;
    gfloat *float_p = src_value->data[1].v_pointer;
    guint8 *uint8_p = dest_value->data[1].v_pointer;

    r = ROUND(float_p[0] * 255.0);
    g = ROUND(float_p[1] * 255.0);
    b = ROUND(float_p[2] * 255.0);

    uint8_p[0] = CLAMP(r, 0, 255);
    uint8_p[1] = CLAMP(g, 0, 255);
    uint8_p[2] = CLAMP(b, 0, 255);
  }
}

static void 
transform_rgba_float_rgb_float(const GValue *src_value,
                              GValue *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    gfloat *src_float_p = src_value->data[1].v_pointer;
    gfloat *dest_float_p = dest_value->data[1].v_pointer;

    dest_float_p[0] = src_float_p[0];
    dest_float_p[1] = src_float_p[1];
    dest_float_p[2] = src_float_p[2];
  }
}

static void 
transform_rgb_float_rgba_float(const GValue *src_value,
                              GValue *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    gfloat *src_float_p = src_value->data[1].v_pointer;
    gfloat *dest_float_p = dest_value->data[1].v_pointer;

    dest_float_p[0] = src_float_p[0];
    dest_float_p[1] = src_float_p[1];
    dest_float_p[2] = src_float_p[2];
    dest_float_p[3] = 1.0;
  }
}

static void 
transform_rgb_uint8_rgb_float(const GValue *src_value,
                              GValue *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  { 
    guint8 *uint8_p = src_value->data[1].v_pointer;
    gfloat *float_p = dest_value->data[1].v_pointer;

    float_p[0] = uint8_p[0]/255.0;
    float_p[1] = uint8_p[1]/255.0;
    float_p[2] = uint8_p[2]/255.0;
  }
}

void
gegl_pixel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_FLOAT, 
                                  GEGL_TYPE_PIXEL_RGB_UINT8, 
                                  transform_rgb_float_rgb_uint8);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGBA_FLOAT, 
                                  GEGL_TYPE_PIXEL_RGB_FLOAT, 
                                  transform_rgba_float_rgb_float);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_FLOAT, 
                                  GEGL_TYPE_PIXEL_RGBA_FLOAT, 
                                  transform_rgb_float_rgba_float);
  g_value_register_transform_func(GEGL_TYPE_PIXEL_RGB_UINT8, 
                                  GEGL_TYPE_PIXEL_RGB_FLOAT, 
                                  transform_rgb_uint8_rgb_float);
}

gpointer
g_value_pixel_get_data(GValue * value)
{
  g_return_val_if_fail (g_type_is_a(G_VALUE_TYPE (value), GEGL_TYPE_PIXEL), NULL);

  return value->data[1].v_pointer;
}

GeglColorModel *
g_value_pixel_get_color_model(GValue * value)
{
  g_return_val_if_fail (g_type_is_a(G_VALUE_TYPE (value), GEGL_TYPE_PIXEL), NULL);

  return gegl_color_model_instance(value->data[0].v_pointer);
}
