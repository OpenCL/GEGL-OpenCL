#include "gegl-color.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-image-iterator.h"
#include "gegl-image-data.h"
#include "gegl-pixel-value-types.h"
#include "gegl-pixel-data.h"
#include "gegl-scalar-data.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_LAST 
};

static void class_init (GeglColorClass * klass);
static void init (GeglColor * self, GeglColorClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void compute_have_rect (GeglImageOp * image_op);
static void compute_color_model (GeglImageOp * image_op);

static GeglScanlineFunc get_scanline_func(GeglNoInput * no_input, GeglColorSpaceType space, GeglChannelSpaceType type);

static void color_uint8 (GeglFilter * filter, GeglImageIterator ** iters, gint width);
static void color_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);
static void color_u16 (GeglFilter * filter, GeglImageIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_color_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NO_INPUT, 
                                     "GeglColor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglColorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglImageOpClass *image_op_class = GEGL_IMAGE_OP_CLASS(klass);
  GeglNoInputClass *no_input_class = GEGL_NO_INPUT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  no_input_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  image_op_class->compute_have_rect = compute_have_rect;
  image_op_class->compute_have_rect = compute_have_rect;
  image_op_class->compute_color_model = compute_color_model;

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
    gegl_param_spec_pixel_rgb_float ("pixel-rgb-float",
                                     "Pixel-Rgb-Float",
                                     "The pixel-rgb-float pixel",
                                     0.0, 1.0,
                                     0.0, 1.0,
                                     0.0, 1.0,
                                     0.0, 0.0, 0.0,
                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
    gegl_param_spec_pixel_rgb_uint8 ("pixel-rgb-uint8",
                                     "Pixel-Rgb-Uint8",
                                     "The rgb-uint8 pixel",
                                     0, 255,
                                     0, 255,
                                     0, 255,
                                     0, 0, 0,
                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                   g_param_spec_int ("width",
                                     "Width",
                                     "The width of the constant image",
                                     0,
                                     G_MAXINT,
                                     GEGL_DEFAULT_WIDTH,
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                   g_param_spec_int ("height",
                                     "Height",
                                     "The height of the constant image",
                                     0,
                                     G_MAXINT,
                                     GEGL_DEFAULT_HEIGHT,
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_READWRITE));
}

static void 
init (GeglColor * self, 
      GeglColorClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_PIXEL_DATA, "pixel");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "width");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "height");
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}


static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglColor *self = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    case PROP_WIDTH:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "width");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    case PROP_HEIGHT:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "height");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColor *self = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      gegl_op_set_input_data_value(GEGL_OP(self), "pixel", value);
      break;
    case PROP_WIDTH:
      gegl_op_set_input_data_value(GEGL_OP(self), "width", value);
      break;
    case PROP_HEIGHT:
      gegl_op_set_input_data_value(GEGL_OP(self), "height", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
compute_have_rect(GeglImageOp * image_op) 
{ 
  GeglColor * self = GEGL_COLOR(image_op);

  GeglData *output_data = 
    gegl_op_get_output_data(GEGL_OP(self), "output-image");
  GeglImageData *output_image_data = GEGL_IMAGE_DATA(output_data);

  GValue *width_data_value;
  GValue *height_data_value;
  gint width;
  gint height;
  GeglRect have_rect;

  width_data_value = gegl_op_get_input_data_value(GEGL_OP(self), "width");
  width = g_value_get_int(width_data_value);

  height_data_value = gegl_op_get_input_data_value(GEGL_OP(self), "height");
  height = g_value_get_int(height_data_value);

  gegl_rect_set(&have_rect, 0,0, width, height);
  gegl_image_data_set_rect(output_image_data,&have_rect);
}

static void
compute_color_model (GeglImageOp * image_op)
{
  GeglData *output_data = 
    gegl_op_get_output_data(GEGL_OP(image_op), "output-image");
  GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(image_op), "pixel");
  GeglColorModel *pixel_cm = g_value_pixel_get_color_model(data_value);

  gegl_color_data_set_color_model(GEGL_COLOR_DATA(output_data), pixel_cm);
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  color_uint8, 
  color_float, 
  color_u16 
};

static GeglScanlineFunc
get_scanline_func(GeglNoInput * no_input,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
color_float(GeglFilter * filter,              
            GeglImageIterator ** iters,        
            gint width)                       
{                                                                       
  GeglColor * self = GEGL_COLOR(filter);
  GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
  gfloat *pixel = (gfloat*)g_value_pixel_get_data(data_value);
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

#if 0 
    g_print("pixel color called %f %f %f \n", pixel[0], pixel[1], pixel[2]);
    g_print("color dest addresses are %p %p %p \n", d0, d1, d2);
#endif

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}                                                                       

static void                                                            
color_u16 (GeglFilter * filter,              
           GeglImageIterator ** iters,        
           gint width)                       
{
  GeglColor * self = GEGL_COLOR(filter);
  GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
  guint16 *pixel = (guint16*)g_value_pixel_get_data(data_value);
  guint16 **d = (guint16**)gegl_image_iterator_color_channels(iters[0]);
  guint16 *da = (guint16*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    guint16 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint16 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint16 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}

static void                                                            
color_uint8 (GeglFilter * filter,              
         GeglImageIterator ** iters,        
         gint width)                       
{
  GeglColor * self = GEGL_COLOR(filter);
  GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
  guint8 *pixel = (guint8*)g_value_pixel_get_data(data_value);
  guint8 **d = (guint8**)gegl_image_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}
