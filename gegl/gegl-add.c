#include "gegl-add.h"
#include "gegl-image-iterator.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-pixel-data.h"

enum
{
  PROP_0, 
  PROP_CONSTANT_RGB_FLOAT,
  PROP_CONSTANT_RGB_UINT8,
  PROP_LAST 
};

static void class_init (GeglAddClass * klass);
static void init (GeglAdd * self, GeglAddClass * klass);

static void finalize (GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void validate_inputs  (GeglFilter *filter, GList *collected_input_data_list);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void add_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);
static void add_uint8 (GeglFilter * filter, GeglImageIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_add_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglAddClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglAdd),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglAdd", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglAddClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->validate_inputs = validate_inputs;

  unary_class->get_scanline_func = get_scanline_func;

  g_object_class_install_property (gobject_class, PROP_CONSTANT_RGB_FLOAT,
         gegl_param_spec_pixel_rgb_float ("constant-rgb-float",
                                          "ConstantRgbFloat",
                                          "The rgb float constant",
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                          -G_MAXFLOAT, G_MAXFLOAT,
                                           0.0, 0.0, 0.0,
                                           G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CONSTANT_RGB_UINT8,
         gegl_param_spec_pixel_rgb_uint8 ("constant-rgb-uint8",
                                          "ConstantRgbUint8",
                                          "The rgb uint8 constant",
                                          0, 255,
                                          0, 255,
                                          0, 255,
                                          0, 0, 0,
                                          G_PARAM_READWRITE));
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_PIXEL_DATA, "constant");
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
  GeglAdd *self = GEGL_ADD(gobject);
  switch (prop_id)
  {
    case PROP_CONSTANT_RGB_FLOAT:
    case PROP_CONSTANT_RGB_UINT8:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "constant");
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
  GeglAdd *self = GEGL_ADD(gobject);
  switch (prop_id)
  {
    case PROP_CONSTANT_RGB_FLOAT:
    case PROP_CONSTANT_RGB_UINT8:
      gegl_op_set_input_data_value(GEGL_OP(self), "constant", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                  GList *collected_input_data_list)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_input_data_list);

  {
    GeglData * output_data = 
        gegl_op_get_output_data(GEGL_OP(filter), "output-image");
    GeglData * input_data = 
        gegl_op_get_input_data(GEGL_OP(filter), "constant");

    GeglColorModel *color_model = 
      gegl_color_data_get_color_model(GEGL_COLOR_DATA(output_data));


    /* Set the desired color model for the input color data */
    gegl_color_data_set_color_model(GEGL_COLOR_DATA(input_data), color_model);

    /* Now should check to see if the collected input data has the right
       color model. */
    {
      gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "constant");
      GeglData *input_data = (GeglData*)g_list_nth_data(collected_input_data_list, index);
      if(input_data)
       {
         /* Convert the pixel to the right color model */
       }
    } 
  }
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  add_uint8, 
  add_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglUnary * unary,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
add_float (GeglFilter * filter,              
           GeglImageIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(self), "constant"); 
  gfloat* data = (gfloat*)g_value_pixel_get_data(value);

  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(iters[1]);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  gint alpha_mask = 0x0;

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 


  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    gfloat *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    gfloat *a1 = (a_color_chans > 1) ? a[1]: NULL;
    gfloat *a2 = (a_color_chans > 2) ? a[2]: NULL;

    switch(d_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
                *d1++ = *a1++ + data[1];
                *d2++ = *a2++ + data[2];
                *da++ = *aa++ + data[3];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
                *d1++ = *a1++ + data[1];
                *d2++ = *a2++ + data[2];
              }
          break;
        case 2: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
                *d1++ = *a1++ + data[1];
                *da++ = *aa++ + data[3];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
                *d1++ = *a1++ + data[1];
              }
          break;
        case 1: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
                *da++ = *aa++ + data[3];
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = *a0++ + data[0];
              }
          break;
      }
  }

  g_free(d);
  g_free(a);
}                                                                       

static void                                                            
add_uint8 (GeglFilter * filter,              
           GeglImageIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(self), "constant"); 
  guint8* data = (guint8*)g_value_pixel_get_data(value);

  guint8 **d = (guint8**)gegl_image_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  guint8 **a = (guint8**)gegl_image_iterator_color_channels(iters[1]);
  guint8 *aa = (guint8*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  gint alpha_mask = 0x0;

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    guint8 *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    guint8 *a1 = (a_color_chans > 1) ? a[1]: NULL;
    guint8 *a2 = (a_color_chans > 2) ? a[2]: NULL;

    switch(d_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
                *d1++ = CLAMP(*a1 + data[1], 0, 255); a1++;
                *d2++ = CLAMP(*a2 + data[2], 0, 255); a2++;
                *da++ = CLAMP(*aa + data[3], 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   

                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
                *d1++ = CLAMP(*a1 + data[1], 0, 255); a1++;
                *d2++ = CLAMP(*a2 + data[2], 0, 255); a2++;
              }
          break;
        case 2:
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
                *d1++ = CLAMP(*a1 + data[1], 0, 255); a1++;
                *da++ = CLAMP(*aa + data[3], 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
                *d1++ = CLAMP(*a1 + data[1], 0, 255); a1++;
              }
          break;
        case 1:
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
                *da++ = CLAMP(*aa + data[3], 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP(*a0 + data[0], 0, 255); a0++;
              }
        break;
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
