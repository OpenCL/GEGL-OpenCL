#include "gegl-mult.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-scalar-data.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"
#include <string.h>

enum
{
  PROP_0, 
  PROP_MULT0,
  PROP_MULT1,
  PROP_MULT2,
  PROP_MULT3,
  PROP_LAST 
};

static void class_init (GeglMultClass * klass);
static void init (GeglMult * self, GeglMultClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglScanlineFunc get_scanline_function(GeglUnary * unary, GeglColorModel *cm);

static void mult_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);
static void mult_uint8 (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMult),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMultClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  unary_class->get_scanline_function = get_scanline_function;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_MULT0,
               g_param_spec_float ("mult0",
                                   "Mult0",
                                   "The multiplier of channel 0",
                                   -G_MAXFLOAT, G_MAXFLOAT,
                                   1.0,
                                   G_PARAM_CONSTRUCT |
                                   G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MULT1,
               g_param_spec_float ("mult1",
                                   "Mult1",
                                   "The multiplier of channel 1",
                                   -G_MAXFLOAT, G_MAXFLOAT,
                                   1.0,
                                   G_PARAM_CONSTRUCT |
                                   G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MULT2,
               g_param_spec_float ("mult2",
                                   "Mult2",
                                   "The multiplier of channel 2",
                                   -G_MAXFLOAT, G_MAXFLOAT,
                                   1.0,
                                   G_PARAM_CONSTRUCT |
                                   G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MULT3,
               g_param_spec_float ("mult3",
                                   "Mult3",
                                   "The multiplier of channel 3",
                                   -G_MAXFLOAT, G_MAXFLOAT,
                                   1.0,
                                   G_PARAM_CONSTRUCT |
                                   G_PARAM_READWRITE));
}

static void 
init (GeglMult * self, 
      GeglMultClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "mult0");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "mult1");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "mult2");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "mult3");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMult *self = GEGL_MULT(gobject);
  switch (prop_id)
  {
    case PROP_MULT0:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult0");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
      break;
    case PROP_MULT1:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult1");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
      break;
    case PROP_MULT2:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult2");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
      break;
    case PROP_MULT3:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult3");
        g_value_set_float(value, g_value_get_float(data_value));  
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
  GeglMult *self = GEGL_MULT(gobject);
  switch (prop_id)
  {
    case PROP_MULT0:
      gegl_op_set_input_data_value(GEGL_OP(self), "mult0", value);
      break;
    case PROP_MULT1:
      gegl_op_set_input_data_value(GEGL_OP(self), "mult1", value);
      break;
    case PROP_MULT2:
      gegl_op_set_input_data_value(GEGL_OP(self), "mult2", value);
      break;
    case PROP_MULT3:
      gegl_op_set_input_data_value(GEGL_OP(self), "mult3", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static GeglScanlineFunc
get_scanline_function(GeglUnary * unary,
                      GeglColorModel *cm)
{
  gchar *name = gegl_color_model_name(cm);

  if(!strcmp(name, "rgb-float"))
    return mult_float;
  else if(!strcmp(name, "rgba-float"))
    return mult_float;
  else if(!strcmp(name, "rgb-uint8"))
    return mult_uint8;
  else if(!strcmp(name, "rgba-uint8"))
    return mult_uint8;

  return NULL;
}


static void                                                            
mult_float (GeglFilter * filter,              
            GeglScanlineProcessor *processor,
            gint width)                       
{                                                                       
  GeglMult * self = GEGL_MULT(filter);

  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(dest);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);

  GeglImageIterator *source = 
    gegl_scanline_processor_lookup_iterator(processor, "source");
  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(source);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(source);
  gint a_color_chans = gegl_image_iterator_get_num_colors(source);

  GValue *mult0_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult0");
  GValue *mult1_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult1");
  GValue *mult2_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult2");
  GValue *mult3_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult3");

  gint alpha_mask = 0x0;

  gfloat mult[4];
  mult[0] = g_value_get_float(mult0_value); 
  mult[1] = g_value_get_float(mult1_value); 
  mult[2] = g_value_get_float(mult2_value); 
  mult[3] = g_value_get_float(mult3_value); 

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    gfloat *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    gfloat *a1 = (a_color_chans > 1) ? a[1]: NULL;
    gfloat *a2 = (a_color_chans > 2) ? a[2]: NULL;

    /* This needs to actually match multipliers to channels returned */

    switch(d_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
                *d1++ = mult[1] * *a1++;
                *d2++ = mult[2] * *a2++;
                *da++ = mult[3] * *aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
                *d1++ = mult[1] * *a1++;
                *d2++ = mult[2] * *a2++;
              }
          break;
        case 2: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
                *d1++ = mult[1] * *a1++;
                *da++ = mult[3] * *aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
                *d1++ = mult[1] * *a1++;
              }
          break;
        case 1: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
                *da++ = mult[3] * *aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = mult[0] * *a0++;
              }
          break;
      }
  }

  g_free(d);
  g_free(a);
}                                                                       

static void                                                            
mult_uint8 (GeglFilter * filter,              
            GeglScanlineProcessor *processor,
            gint width)                       
{                                                                       
  GeglMult * self = GEGL_MULT(filter);

  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  guint8 **d = (guint8**)gegl_image_iterator_color_channels(dest);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);

  GeglImageIterator *source = 
    gegl_scanline_processor_lookup_iterator(processor, "source");
  guint8 **a = (guint8**)gegl_image_iterator_color_channels(source);
  guint8 *aa = (guint8*)gegl_image_iterator_alpha_channel(source);
  gint a_color_chans = gegl_image_iterator_get_num_colors(source);

  GValue *mult0_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult0");
  GValue *mult1_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult1");
  GValue *mult2_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult2");
  GValue *mult3_value = gegl_op_get_input_data_value(GEGL_OP(self), "mult3");

  gint alpha_mask = 0x0;

  gfloat mult[4];
  mult[0] = g_value_get_float(mult0_value); 
  mult[1] = g_value_get_float(mult1_value); 
  mult[2] = g_value_get_float(mult2_value); 
  mult[3] = g_value_get_float(mult3_value); 

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    guint8 *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    guint8 *a1 = (a_color_chans > 1) ? a[1]: NULL;
    guint8 *a2 = (a_color_chans > 2) ? a[2]: NULL;

    /* This needs to actually match multipliers to channels returned */

    switch(d_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
                *d1++ = CLAMP((gint)(mult[1] * *a1 + .5), 0, 255); a1++;
                *d2++ = CLAMP((gint)(mult[2] * *a2 + .5), 0, 255); a2++;
                *da++ = CLAMP((gint)(mult[3] * *aa + .5), 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
                *d1++ = CLAMP((gint)(mult[1] * *a1 + .5), 0, 255); a1++;
                *d2++ = CLAMP((gint)(mult[2] * *a2 + .5), 0, 255); a2++;
              }
          break;
        case 2: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
                *d1++ = CLAMP((gint)(mult[1] * *a1 + .5), 0, 255); a1++;
                *da++ = CLAMP((gint)(mult[3] * *aa + .5), 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
                *d1++ = CLAMP((gint)(mult[1] * *a1 + .5), 0, 255); a1++;
              }
          break;
        case 1: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
                *da++ = CLAMP((gint)(mult[3] * *aa + .5), 0, 255); aa++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = CLAMP((gint)(mult[0] * *a0 + .5), 0, 255); a0++;
              }
          break;
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
