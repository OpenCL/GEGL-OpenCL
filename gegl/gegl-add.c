#include "gegl-add.h"
#include "gegl-image-iterator.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-color.h"
#include "gegl-color-data.h"
#include <string.h>

enum
{
  PROP_0, 
  PROP_CONSTANT,
  PROP_LAST 
};

static void class_init (GeglAddClass * klass);
static void init (GeglAdd * self, GeglAddClass * klass);

static void finalize (GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void validate_inputs  (GeglFilter *filter, GArray *collected_data);

static GeglScanlineFunc get_scanline_function(GeglUnary * unary, GeglColorModel *cm);

static void add_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);
static void add_uint8 (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

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
        NULL
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

  unary_class->get_scanline_function = get_scanline_function;

  g_object_class_install_property (gobject_class, PROP_CONSTANT,
               g_param_spec_object ("constant",
                                    "Constant",
                                    "Constant color added to each pixel.",
                                     GEGL_TYPE_COLOR,
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_READWRITE));
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_COLOR_DATA, "constant");
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
    case PROP_CONSTANT:
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
    case PROP_CONSTANT:
      gegl_op_set_input_data_value(GEGL_OP(self), "constant", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                  GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);

  {
    GeglData * output_data = gegl_op_get_output_data(GEGL_OP(filter), "dest");
    GeglData * input_data = gegl_op_get_input_data(GEGL_OP(filter), "constant");

    /* Nothing to do here */
  }
}

static GeglScanlineFunc
get_scanline_function(GeglUnary * unary,
                      GeglColorModel *cm)
{
  gchar *name = gegl_color_model_name(cm);

  if(!strcmp(name, "rgb-float"))
    return add_float;
  else if(!strcmp(name, "rgba-float"))
    return add_float;
  else if(!strcmp(name, "rgb-uint8"))
    return add_uint8;
  else if(!strcmp(name, "rgba-uint8"))
    return add_uint8;

  return NULL;
}


static void                                                            
add_float (GeglFilter * filter,              
           GeglScanlineProcessor *processor,
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

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

  gint num_components;
  GeglData *color_data = gegl_op_get_input_data(GEGL_OP(self), "constant");
  gfloat *data = gegl_color_data_get_components(GEGL_COLOR_DATA(color_data), &num_components);

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

  g_free(data);
}                                                                       

static void                                                            
add_uint8 (GeglFilter * filter,              
           GeglScanlineProcessor *processor,
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

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

  gint num_components;
  GeglData *color_data = gegl_op_get_input_data(GEGL_OP(self), "constant");
  gfloat *float_data = gegl_color_data_get_components(GEGL_COLOR_DATA(color_data), &num_components);

  guint8 *data = g_new(guint8, num_components);
  gint i;
  gint alpha_mask = 0x0;

  /* This should be somewhere else, not here. */
  for(i=0; i < num_components; i++)
    {
      gint channel = ROUND(255 * float_data[i]); 
      data[i] = CLAMP(channel, 0, 255);
    }

  g_free(float_data);

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
  g_free(data);
}                                                                       
