#include "gegl-fill.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color.h"
#include "gegl-image-iterator.h"
#include "gegl-image-data.h"
#include "gegl-color-data.h"
#include "gegl-string-data.h"
#include "gegl-image-op-interface.h"
#include "gegl-scalar-data.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"
#include <string.h>


enum
{
  PROP_0, 
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_FILL_COLOR,
  PROP_IMAGE_DATA_TYPE,
  PROP_LAST 
};

static void class_init (GeglFillClass * klass);
static void init (GeglFill * self, GeglFillClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void image_op_interface_init (gpointer ginterface, gpointer interface_data);
static void compute_have_rect (GeglImageOpInterface *interface);
static void compute_color_model (GeglImageOpInterface *interface);

static GeglScanlineFunc get_scanline_function(GeglNoInput * no_input, GeglColorModel *cm);

static void fill_uint8 (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);
static void fill_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);
static void fill_u16 (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_fill_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFillClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFill),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      static const GInterfaceInfo image_op_interface_info = 
      { 
         (GInterfaceInitFunc) image_op_interface_init,
         NULL,  
         NULL
      };

      type = g_type_register_static (GEGL_TYPE_NO_INPUT, 
                                     "GeglFill", 
                                     &typeInfo, 
                                     0);

      g_type_add_interface_static (type, 
                                   GEGL_TYPE_IMAGE_OP_INTERFACE,
                                   &image_op_interface_info);
    }
    return type;
}

static void 
class_init (GeglFillClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglNoInputClass *no_input_class = GEGL_NO_INPUT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  no_input_class->get_scanline_function = get_scanline_function;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

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

  g_object_class_install_property (gobject_class, PROP_FILL_COLOR,
               g_param_spec_object ("fill-color",
                                    "FillColor",
                                    "The fill color.",
                                     GEGL_TYPE_COLOR,
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE_DATA_TYPE,
               g_param_spec_string ("image-data-type",
                                    "ImageDataType",
                                    "The color model of the image data.",
                                    "rgb-float",
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_READWRITE));

}

static void
image_op_interface_init (gpointer ginterface,
                         gpointer interface_data)
{
  GeglImageOpInterfaceClass *interface = ginterface;

  g_assert (G_TYPE_FROM_INTERFACE (interface) == GEGL_TYPE_IMAGE_OP_INTERFACE);

  interface->compute_have_rect = compute_have_rect;
  interface->compute_color_model = compute_color_model;
}

static void 
init (GeglFill * self, 
      GeglFillClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_COLOR_DATA, "fill-color");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_STRING_DATA, "image-data-type");
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
  GeglFill *self = GEGL_FILL(gobject);
  switch (prop_id)
  {
    case PROP_FILL_COLOR:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "fill-color");
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
    case PROP_IMAGE_DATA_TYPE:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "image-data-type");
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
  GeglFill *self = GEGL_FILL(gobject);
  switch (prop_id)
  {
    case PROP_FILL_COLOR:
      gegl_op_set_input_data_value(GEGL_OP(self), "fill-color", value);
      break;
    case PROP_WIDTH:
      gegl_op_set_input_data_value(GEGL_OP(self), "width", value);
      break;
    case PROP_HEIGHT:
      gegl_op_set_input_data_value(GEGL_OP(self), "height", value);
      break;
    case PROP_IMAGE_DATA_TYPE:
      gegl_op_set_input_data_value(GEGL_OP(self), "image-data-type", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
compute_have_rect (GeglImageOpInterface   *interface)
{
  GeglFill * self = GEGL_FILL(interface);

  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
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
compute_color_model (GeglImageOpInterface   *interface)
{
  GeglFill *self = GEGL_FILL(interface);
  GeglData *output_data = gegl_op_get_output_data(GEGL_OP(self), "dest");
  GValue *string_value = gegl_op_get_input_data_value(GEGL_OP(self), "image-data-type");
  gchar *color_model_name = (gchar*)g_value_get_string(string_value);
  GeglColorModel *cm = gegl_color_model_instance(color_model_name);
  gegl_image_data_set_color_model(GEGL_IMAGE_DATA(output_data), cm);
}

static GeglScanlineFunc
get_scanline_function(GeglNoInput * no_input,
                      GeglColorModel *cm)
{
  gchar *name = gegl_color_model_name(cm);

  if(!strcmp(name, "rgb-float"))
    return fill_float;
  else if(!strcmp(name, "rgba-float"))
    return fill_float;
  else if(!strcmp(name, "rgb-uint8"))
    return fill_uint8;
  else if(!strcmp(name, "rgba-uint8"))
    return fill_uint8;

  return NULL;
}

static void                                                            
fill_float(GeglFilter * filter,              
            GeglScanlineProcessor *processor,
            gint width)                       
{                                                                       
  GeglFill * self = GEGL_FILL(filter);

  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(dest);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  gint num_components;
  GeglData *data = gegl_op_get_input_data(GEGL_OP(self), "fill-color");
  gfloat *pixel = gegl_color_data_get_components(GEGL_COLOR_DATA(data), &num_components);

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    switch(d_color_chans)
      {
        case 3: 
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *d2++ = pixel[2];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *d2++ = pixel[2];
              }
          break;
        case 2:
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
              }
          break;
        case 1:
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
              }
          break;
      }
  }

  g_free(pixel);
  g_free(d);
}                                                                       


static void                                                            
fill_uint8 (GeglFilter * filter,              
             GeglScanlineProcessor *processor,
             gint width)                       
{
  GeglFill * self = GEGL_FILL(filter);

  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  guint8 **d = (guint8**)gegl_image_iterator_color_channels(dest);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  gint num_components;
  GeglData *data = gegl_op_get_input_data(GEGL_OP(self), "fill-color");
  gfloat *pix = gegl_color_data_get_components(GEGL_COLOR_DATA(data), &num_components);

  guint8 *pixel = g_new(guint8, num_components);
  gint i;

  /* This should be somewhere else, not here. */
  for(i=0; i < num_components; i++)
    {
      gint channel = ROUND(255 * pix[i]); 
      pixel[i] = CLAMP(channel, 0, 255);
    }
   
  g_free(pix);

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    switch(d_color_chans)
      {
        case 3: 
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *d2++ = pixel[2];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *d2++ = pixel[2];
              }
          break;
        case 2: 
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *d1++ = pixel[1];
              }
          break;
        case 1: 
          if(has_alpha)
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
                *da++ = pixel[alpha];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = pixel[0];
              }
          break;
      }
  }

  g_free(d);
  g_free(pixel);
}
