#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-color.h"
#include <string.h>

static void class_init (GeglColorModelGrayU8Class * klass);
static void init (GeglColorModelGrayU8 * self, GeglColorModelGrayU8Class * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static GeglColorModelType color_model_type (GeglColorModel * self_color_model);
static void set_color (GeglColorModel * self_color_model, GeglColor * color, GeglColorConstant constant);
static char * get_convert_interface_name (GeglColorModel * self_color_model);
static void convert_to_xyz (GeglColorModel * self_color_model, gfloat ** xyz_data, guchar ** data, gint width);
static void convert_from_xyz (GeglColorModel * self_color_model, guchar ** data, gfloat ** xyz_data, gint width);
static void from_gray_float (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);
static void from_rgb_u8 (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);
static void from_gray_u8 (GeglColorModel * self_color_model, GeglColorModel * src_color_model, guchar ** data, guchar ** src_data, gint width);
static gpointer parent_class = NULL;

GType
gegl_color_model_gray_u8_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorModelGrayU8Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorModelGrayU8),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_MODEL_GRAY, 
                                     "GeglColorModelGrayU8", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglColorModelGrayU8Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglColorModelClass *color_model_class = GEGL_COLOR_MODEL_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor;

  color_model_class->color_model_type = color_model_type;
  color_model_class->set_color = set_color;
  color_model_class->get_convert_interface_name = get_convert_interface_name;
  color_model_class->convert_to_xyz = convert_to_xyz;
  color_model_class->convert_from_xyz = convert_from_xyz;
  return;
}

static void 
init (GeglColorModelGrayU8 * self, 
      GeglColorModelGrayU8Class * klass)
{
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL (self);

  self_color_model->data_type = GEGL_U8;
  self_color_model->channel_data_type_name = NULL;
  self_color_model->bytes_per_channel = sizeof(guint8);

  /* These are the color models we can convert from directly */

    gegl_object_add_interface (GEGL_OBJECT(self), "FromGrayU8", from_gray_u8);
    gegl_object_add_interface (GEGL_OBJECT(self), "FromRgbU8", from_rgb_u8);
    gegl_object_add_interface (GEGL_OBJECT(self), "FromGrayFloat", from_gray_float);

  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglColorModel *self_color_model = GEGL_COLOR_MODEL(gobject);
  self_color_model->bytes_per_pixel = self_color_model->bytes_per_channel * 
                                      self_color_model->num_channels;
  return gobject;
}

static GeglColorModelType 
color_model_type (GeglColorModel * self_color_model)
{
  return self_color_model->has_alpha ?  
             GEGL_COLOR_MODEL_TYPE_GRAYA_U8: 
             GEGL_COLOR_MODEL_TYPE_GRAY_U8;
}

static void 
set_color (GeglColorModel * self_color_model, 
           GeglColor * color, 
           GeglColorConstant constant)
{
  GeglChannelValue * channel_values = gegl_color_get_channel_values(color); 
  gboolean has_alpha = gegl_color_model_has_alpha (self_color_model); 
  GeglColorModelGray * cm_gray = GEGL_COLOR_MODEL_GRAY(self_color_model);
  
  gint g = gegl_color_model_gray_get_gray_index (cm_gray);
  gint a = gegl_color_model_alpha_channel (GEGL_COLOR_MODEL(cm_gray));

  switch (constant) 
    { 
      case GEGL_COLOR_WHITE:
        channel_values[g].u8 = 255;
        if (has_alpha)
          channel_values[a].u8 = 255;
        break;
      case GEGL_COLOR_BLACK:
        channel_values[g].u8 = 0;
        if (has_alpha)
          channel_values[a].u8 = 255;
        break;
      case GEGL_COLOR_RED:
        channel_values[g].u8 = CLAMP(ROUND(0.3 * 255),0,255);
        if (has_alpha)
          channel_values[a].f = 255;
        break;
      case GEGL_COLOR_GREEN:
        channel_values[g].u8 = CLAMP(ROUND(0.59 * 255),0,255);
        if (has_alpha)
          channel_values[a].u8 = 255;
        break;
      case GEGL_COLOR_BLUE:
        channel_values[g].u8 = CLAMP(ROUND(0.11 * 255),0,255);
        if (has_alpha)
          channel_values[a].u8 = 255;
        break;
      case GEGL_COLOR_GRAY:
      case GEGL_COLOR_HALF_WHITE:
        channel_values[g].u8 = 127;
        if (has_alpha)
          channel_values[a].u8 = 255;
        break;
      case GEGL_COLOR_WHITE_TRANSPARENT:
        channel_values[g].u8 = 255;
        if (!has_alpha)
          channel_values[a].u8 = 0;
        break;
      case GEGL_COLOR_TRANSPARENT:
      case GEGL_COLOR_BLACK_TRANSPARENT:
        channel_values[g].u8 = 0;
        if (has_alpha)
          channel_values[a].u8 = 0;
        break;
    }
}

static char * 
get_convert_interface_name (GeglColorModel * self_color_model)
{
  return g_strdup ("FromGrayU8"); 
}

static void 
convert_to_xyz (GeglColorModel * self_color_model, 
                gfloat ** xyz_data, 
                guchar ** data, 
                gint width)
{
  /* convert from u8 gray to float xyz */

  gfloat m00 = 0.412453;
  gfloat m10 = 0.357580;
  gfloat m20 = 0.180423;
  gfloat m01 = 0.212671;
  gfloat m11 = 0.715160;
  gfloat m21 = 0.072169;
  gfloat m02 = 0.019334;
  gfloat m12 = 0.119193;
  gfloat m22 = 0.950227;
                            
  guint8 *g; 
  guint8 *a = NULL;
  gfloat G;
  gfloat *x, *y, *z; 
  gfloat *xyz_a = NULL;
  gfloat tmp= 1.0/255.0;

  gboolean has_alpha = gegl_color_model_has_alpha(self_color_model);

  g = (guint8*)data[0];
  x = xyz_data[0];
  y = xyz_data[1];
  z = xyz_data[2];

  if (has_alpha)
    {
     a = (guint8*)data[1];
     xyz_a = xyz_data[3];
    }

  while (width--)
    {
     G = *g++ * tmp;
     *x++ = (m00 + m10 + m20) * G;
     *y++ = (m01 + m11 + m21) * G;
     *z++ = (m02 + m12 + m22) * G;
     if (has_alpha)
       *xyz_a++ = *a++ * tmp;
    }
}

static void 
convert_from_xyz (GeglColorModel * self_color_model, 
                  guchar ** data, 
                  gfloat ** xyz_data, 
                  gint width)
{
  /* convert from float xyz to u8 gray */

  /*
    __  __      __                               __  __  __
    | R  |      | 3.240479   -1.537150   -0.498535 | | X  |
    | G  |   =  |-0.969256    1.875992    0.041556 | | Y  |
    | B  |      | 0.055648   -0.204043    1.057311 | | Z  |
    --  --      ---                             ---- --  --
  */
  gfloat m00 = 3.240479;
  gfloat m10 = -1.537150;
  gfloat m20 = -0.498535;
  gfloat m01 = -0.969256;
  gfloat m11 = 1.875991;
  gfloat m21 = 0.041556;
  gfloat m02 = 0.055648;
  gfloat m12 = -0.204043;
  gfloat m22 = 1.057311;
                              
  guint8 *g; 
  guint8 *a = NULL;
  gfloat *x, *y, *z; 
  gfloat *xyz_a = NULL; 

  gboolean has_alpha = gegl_color_model_has_alpha(self_color_model);

  g = (guint8*)data[0];

  x = xyz_data[0];
  y = xyz_data[1];
  z = xyz_data[2];

  if (has_alpha)
    {
     a = (guint8*)data[1];
     xyz_a = xyz_data[3];
    }

  while (width--)
    {
       *g  = CLAMP(ROUND((m00 * *x + m10 * *y + m20 * *z) * 255 * 0.3),0,255);
       *g += CLAMP(ROUND((m01 * *x + m11 * *y + m21 * *z) * 255 * 0.59),0,255);
       *g++ += CLAMP(ROUND((m02 * *x + m12 * *y + m22 * *z) * 255 * 0.11),0,255);

       x++;
       y++;
       z++;

       if (has_alpha)
       {
         *a++  = CLAMP(ROUND(*xyz_a * 255),0,255);
         xyz_a++;
       }
    }
}

/**
 * from_gray_float:
 * @self_color_model: a #GeglColorModel (GRAY_U8).
 * @src_color_model:  a #GeglColorModel (GRAY_FLOAT).
 * @data: pointers to the dest gray u8 data.
 * @src_data: pointers to the src gray float data.
 * @width: number of pixels to process.
 *
 * Converts from GRAY_FLOAT to GRAY_U8.
 *
 **/
static void 
from_gray_float (GeglColorModel * self_color_model, 
                 GeglColorModel * src_color_model, 
                 guchar ** data, 
                 guchar ** src_data, 
                 gint width)
{
  guint8          *g;
  guint8          *a = NULL;
  gfloat          *src_g; 
  gfloat          *src_a = NULL;
  gboolean         has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean         src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  
  g = (guint8*)data[0];
  if (has_alpha)
    a = (guint8*)data[1];

  src_g = (gfloat*)src_data[0];
  if (src_has_alpha)
    src_a = (gfloat*)src_data[1];

  while (width--)
    {
       *g++ = CLAMP(ROUND(*src_g * 255),0,255);
       src_g++;

       if (src_has_alpha && has_alpha)
          {
            *a++ = CLAMP(ROUND(*src_a * 255),0,255);
            src_a++;
          }
       else if (has_alpha) 
          *a++ = 255;
    } 
}

/**
 * from_rgb_u8:
 * @self_color_model: a #GeglColorModel (GRAY_U8).
 * @src_color_model:  a #GeglColorModel (RGB_U8).
 * @data: pointers to the dest gray u8 data.
 * @src_data: pointers to the src rgb u8 data.
 * @width: number of pixels to process.
 *
 * Converts from RGB_U8 to GRAY_U8.
 *
 **/
static void 
from_rgb_u8 (GeglColorModel * self_color_model, 
             GeglColorModel * src_color_model, 
             guchar ** data, 
             guchar ** src_data, 
             gint width)
{
  guint8    *g;
  guint8    *a = NULL;
  guint8    *src_r, *src_g, *src_b; 
  guint8    *src_a = NULL;
  gboolean   has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean   src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  
  g = (guint8*)data[0];
  if (has_alpha)
    a = (guint8*)data[1];
  
  src_r = (guint8*) src_data[0];
  src_g = (guint8*) src_data[1];
  src_b = (guint8*) src_data[2];
  if (src_has_alpha)
    src_a = (guint8*) src_data[3];
  
  while (width--)
    {
     *g++ = ROUND(.30 * *src_r++ + .59 * *src_g++ + .11 * *src_b++);

     if (src_has_alpha && has_alpha)
       *a++ = *src_a++;
     else if (has_alpha) 
       *a++ = 255;
    } 
}

/**
 * gegl_color_model_gray_u8_from_gray_u8:
 * @self_color_model: a #GeglColorModel (GRAY_U8).
 * @src_color_model:  a #GeglColorModel (GRAY_U8).
 * @data: pointers to the dest gray u8 data.
 * @src_data: pointers to the src gray u8 data.
 * @width: number of pixels to process.
 *
 * Converts from GRAY_U8 to GRAY_U8.
 *
 **/
static void 
from_gray_u8 (GeglColorModel * self_color_model, 
              GeglColorModel * src_color_model, 
              guchar ** data, 
              guchar ** src_data, 
              gint width)
{
  guint8    *g;
  guint8    *a = NULL;
  guint8    *src_g;
  guint8    *src_a = NULL;
  gboolean   has_alpha = gegl_color_model_has_alpha(self_color_model);
  gboolean   src_has_alpha = gegl_color_model_has_alpha(src_color_model);
  gint       row_bytes = width * gegl_color_model_bytes_per_channel(self_color_model); 
  
  g = (guint8*)data[0];
  if (has_alpha)
    a = (guint8*)data[1];
  
  src_g = (guint8*)src_data[0];
  if (src_has_alpha)
    src_a = (guint8*)src_data[1];
  
  if (src_has_alpha && has_alpha)
    {
      memcpy(g, src_g, row_bytes);  
      memcpy(a, src_a, row_bytes);  
    }
  else if (has_alpha) 
    {
      memcpy(g, src_g, row_bytes);  

      while(width--)
        *a++ = 255;
    }
  else 
    {
      memcpy(g, src_g, row_bytes);  
    }
}
