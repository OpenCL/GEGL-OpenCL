#include "gegl-init.h"
#include "gegl-types.h"
#include "gegl-color-space-rgb.h"
#include "gegl-color-space-gray.h"
#include "gegl-data-space-float.h"
#include "gegl-data-space-u8.h"
#include "gegl-component-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

static gboolean gegl_initialized = FALSE;

static
void
gegl_init_color_spaces(void)
{
  GeglColorSpace * rgb = g_object_new(GEGL_TYPE_COLOR_SPACE_RGB, NULL);
  GeglColorSpace * gray = g_object_new(GEGL_TYPE_COLOR_SPACE_GRAY, NULL);

  gegl_color_space_register(rgb);
  gegl_color_space_register(gray);

  g_object_unref (rgb); 
  g_object_unref (gray); 
}

static
void
gegl_free_color_spaces(void)
{
  GeglColorSpace * rgb = gegl_color_space_instance("rgb");
  GeglColorSpace * gray = gegl_color_space_instance("gray");

  g_object_unref (rgb); 
  g_object_unref (gray); 
}

static
void
gegl_init_data_spaces(void)
{
  GeglDataSpace * flt = g_object_new(GEGL_TYPE_DATA_SPACE_FLOAT, NULL);
  GeglDataSpace * u8 = g_object_new(GEGL_TYPE_DATA_SPACE_U8, NULL);

  gegl_data_space_register(flt);
  gegl_data_space_register(u8);

  g_object_unref (flt); 
  g_object_unref (u8); 
}

static
void
gegl_free_data_spaces(void)
{
  GeglDataSpace * flt = gegl_data_space_instance("float");
  GeglDataSpace * u8 = gegl_data_space_instance("u8");

  g_object_unref (flt); 
  g_object_unref (u8); 
}

static
void
gegl_init_color_models(void)
{
  GeglColorSpace * rgb = gegl_color_space_instance("rgb");
  GeglColorSpace * gray = gegl_color_space_instance("gray");
  GeglDataSpace * flt = gegl_data_space_instance("float");
  GeglDataSpace * u8 = gegl_data_space_instance("u8");

  GeglColorModel * pixel_rgb_float = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                          "color_space", rgb,
                                          "data_space", flt,
                                          NULL);

  GeglColorModel * rgba_float = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                           "color_space", rgb,
                                           "data_space", flt,
                                           "has_alpha", TRUE,
                                           NULL);

  GeglColorModel * gray_float = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                           "color_space", gray,
                                           "data_space", flt,
                                           NULL);

  GeglColorModel * graya_float = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                            "color_space", gray,
                                            "data_space", flt,
                                            "has_alpha", TRUE,
                                            NULL);

  GeglColorModel * rgb_u8 = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                       "color_space", rgb,
                                       "data_space", u8,
                                       NULL);

  GeglColorModel * rgba_u8 = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                        "color_space", rgb,
                                        "data_space", u8,
                                        "has_alpha", TRUE,
                                        NULL);

  GeglColorModel * gray_u8 = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                        "color_space", gray,
                                        "data_space", u8,
                                        NULL);

  GeglColorModel * graya_u8 = g_object_new(GEGL_TYPE_COMPONENT_COLOR_MODEL, 
                                         "color_space", gray,
                                         "data_space", u8,
                                         "has_alpha", TRUE,
                                         NULL);

  gegl_color_model_register(pixel_rgb_float);
  gegl_color_model_register(rgba_float);
  gegl_color_model_register(gray_float);
  gegl_color_model_register(graya_float);
  gegl_color_model_register(rgb_u8);
  gegl_color_model_register(rgba_u8);
  gegl_color_model_register(gray_u8);
  gegl_color_model_register(graya_u8);

  g_object_unref (pixel_rgb_float); 
  g_object_unref (rgba_float); 
  g_object_unref (gray_float); 
  g_object_unref (graya_float); 
  g_object_unref (rgb_u8); 
  g_object_unref (rgba_u8); 
  g_object_unref (gray_u8); 
  g_object_unref (graya_u8); 
}

static
void
gegl_free_color_models(void)
{
  GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
  GeglColorModel * rgba_float = gegl_color_model_instance("rgba-float");
  GeglColorModel * gray_float = gegl_color_model_instance("gray-float");
  GeglColorModel * graya_float = gegl_color_model_instance("graya-float");
  GeglColorModel * rgb_u8 = gegl_color_model_instance("rgb-u8");
  GeglColorModel * rgba_u8 = gegl_color_model_instance("rgba-u8");
  GeglColorModel * gray_u8 = gegl_color_model_instance("gray-u8");
  GeglColorModel * graya_u8 = gegl_color_model_instance("graya-u8");

  g_object_unref (pixel_rgb_float); 
  g_object_unref (rgba_float); 
  g_object_unref (gray_float); 
  g_object_unref (graya_float); 
  g_object_unref (rgb_u8); 
  g_object_unref (rgba_u8); 
  g_object_unref (gray_u8); 
  g_object_unref (graya_u8); 
}

GType GEGL_TYPE_SCALAR = 0;
GType GEGL_TYPE_CHANNEL = 0;
GType GEGL_TYPE_PIXEL = 0;

static void 
gegl_fundamental_types_init(void)
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

#if 0

  g_assert (GEGL_TYPE_SCALAR == 0);
  GEGL_TYPE_SCALAR = G_TYPE_MAKE_FUNDAMENTAL(49);
  type = g_type_register_fundamental (GEGL_TYPE_SCALAR, 
                                      "GeglScalar", 
                                      &info, 
                                      &finfo, 
                                      G_TYPE_FLAG_ABSTRACT);

  g_assert (type == GEGL_TYPE_SCALAR);
#endif

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
gegl_value_types_init (void)
{
  gegl_channel_value_types_init();
  gegl_pixel_value_types_init();
}

static void
gegl_value_transform_init(void) 
{
  gegl_channel_value_transform_init();
  gegl_pixel_value_transform_init();
}

static void
gegl_param_spec_types_init (void)
{
  gegl_channel_param_spec_types_init();
  gegl_pixel_param_spec_types_init();
  gegl_image_data_param_spec_types_init();
}

static
void
gegl_exit(void)
{
  gegl_free_color_models();
  gegl_free_color_spaces();
  gegl_free_data_spaces();
}

void
gegl_init (int *argc, 
           char ***argv)
{
  if (gegl_initialized)
    return;

  gegl_init_color_spaces();
  gegl_init_data_spaces();
  gegl_init_color_models();

  gegl_fundamental_types_init();
  gegl_value_types_init ();
  gegl_value_transform_init (); 
  gegl_param_spec_types_init ();

  g_atexit(gegl_exit);
  gegl_initialized = TRUE;
}
