#include "gegl-min.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglMinClass * klass);
static void init (GeglMin * self, GeglMinClass * klass);

static GeglScanlineFunc get_scanline_func(GeglBinary * binary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void a_min_b_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_min_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMinClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMin),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_BINARY, 
                                     "GeglMin", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMinClass * klass)
{
  GeglBinaryClass *blend_class = GEGL_BINARY_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  blend_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglMin * self, 
      GeglMinClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  a_min_b_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglBinary * binary,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
a_min_b_float (GeglFilter * filter,              
               GeglScanlineProcessor *processor,
               gint width)                       
{                                                                       
  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(dest);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);

  GeglImageIterator *source0 = 
    gegl_scanline_processor_lookup_iterator(processor, "source-0");
  gfloat **b = (gfloat**)gegl_image_iterator_color_channels(source0);
  gfloat *ba = (gfloat*)gegl_image_iterator_alpha_channel(source0);
  gint b_color_chans = gegl_image_iterator_get_num_colors(source0);

  GeglImageIterator *source1 = 
    gegl_scanline_processor_lookup_iterator(processor, "source-1");
  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(source1);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(source1);
  gint a_color_chans = gegl_image_iterator_get_num_colors(source1);

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(filter), "fade"); 
  gfloat fade = g_value_get_float(value);

  gint alpha_mask = 0x0;

  if(ba) 
    alpha_mask |= GEGL_B_ALPHA; 
  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    gfloat *b0 = (b_color_chans > 0) ? b[0]: NULL;   
    gfloat *b1 = (b_color_chans > 1) ? b[1]: NULL;
    gfloat *b2 = (b_color_chans > 2) ? b[2]: NULL;

    gfloat *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    gfloat *a1 = (a_color_chans > 1) ? a[1]: NULL;
    gfloat *a2 = (a_color_chans > 2) ? a[2]: NULL;

    switch(d_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_B_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
                *d1++ = MIN(*a1, fade * *b1); a1++; b1++;
                *d2++ = MIN(*a2, fade * *b2); a2++; b2++;
                *da++ = MIN(*aa, fade * *ba); aa++; ba++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
                *d1++ = MIN(*a1, fade * *b1); a1++; b1++;
                *d2++ = MIN(*a2, fade * *b2); a2++; b2++;
              }
          break;
        case 2: 
          if(alpha_mask == GEGL_A_B_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
                *d1++ = MIN(*a1, fade * *b1); a1++; b1++;
                *da++ = MIN(*aa, fade * *ba); aa++; ba++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
                *d1++ = MIN(*a1, fade * *b1); a1++; b1++;
              }
          break;
        case 1: 
          if(alpha_mask == GEGL_A_B_ALPHA)
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
                *da++ = MIN(*aa, fade * *ba); aa++; ba++;
              }
          else
            while(width--)                                                        
              {                                                                   
                *d0++ = MIN(*a0, fade * *b0); a0++; b0++;
              }
          break;
      }
  }

  g_free(d);
  g_free(b);
  g_free(a);
}                                                                       
