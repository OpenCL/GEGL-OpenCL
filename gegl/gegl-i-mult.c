#include "gegl-i-mult.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-data-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglIMultClass * klass);
static void init (GeglIMult * self, GeglIMultClass * klass);

static GeglScanlineFunc get_scanline_func(GeglBinary * binary, GeglColorSpaceType space, GeglDataSpaceType type);

static void a_imult_b_float (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);
static void a_imult_b_uint8 (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);                       

static gpointer parent_class = NULL;

GType
gegl_i_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglIMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglIMult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_BINARY, 
                                     "GeglIMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglIMultClass * klass)
{
  GeglBinaryClass *blend_class = GEGL_BINARY_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  blend_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglIMult * self, 
      GeglIMultClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  a_imult_b_uint8, 
  a_imult_b_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglBinary * binary,
                  GeglColorSpaceType space,
                  GeglDataSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
a_imult_b_float (GeglFilter * filter,              
                 GeglImageDataIterator ** iters,        
                 gint width)                       
{                                                                       
  GeglBinary * binary = GEGL_BINARY(filter);

  gfloat **d = (gfloat**)gegl_image_data_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);

  gfloat **b = (gfloat**)gegl_image_data_iterator_color_channels(iters[1]);
  gfloat *ba = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[1]);
  gint b_color_chans = gegl_image_data_iterator_get_num_colors(iters[1]);

  gfloat **a = (gfloat**)gegl_image_data_iterator_color_channels(iters[2]);
  gfloat *aa = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[2]);
  gint a_color_chans = gegl_image_data_iterator_get_num_colors(iters[2]);

  gint alpha_mask = 0x0;
  gfloat fade = binary->fade;

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

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = fade * *a2++ * *b2++; 
            case 2: *d1++ = fade * *a1++ * *b1++;
            case 1: *d0++ = fade * *a0++ * *b0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_B_ALPHA)
          {
              *da++ = fade * *aa++ * *ba++;
          }
      }
  }

  g_free(d);
  g_free(b);
  g_free(a);
}                                                                       

static void                                                            
a_imult_b_uint8 (GeglFilter * filter,              
                GeglImageDataIterator ** iters,        
                gint width)                       
{                                                                       
  GeglBinary * binary = GEGL_BINARY(filter);

  guint8 **d = (guint8**)gegl_image_data_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);

  guint8 **b = (guint8**)gegl_image_data_iterator_color_channels(iters[1]);
  guint8 *ba = (guint8*)gegl_image_data_iterator_alpha_channel(iters[1]);
  gint b_color_chans = gegl_image_data_iterator_get_num_colors(iters[1]);

  guint8 **a = (guint8**)gegl_image_data_iterator_color_channels(iters[2]);
  guint8 *aa = (guint8*)gegl_image_data_iterator_alpha_channel(iters[2]);
  gint a_color_chans = gegl_image_data_iterator_get_num_colors(iters[2]);

  gint alpha_mask = 0x0;
  guint8 fade = CLAMP((gint)(binary->fade * 255 + .5), 0, 255); 
  guint t,s;

  if(ba) 
    alpha_mask |= GEGL_B_ALPHA; 
  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    guint8 *b0 = (b_color_chans > 0) ? b[0]: NULL;   
    guint8 *b1 = (b_color_chans > 1) ? b[1]: NULL;
    guint8 *b2 = (b_color_chans > 2) ? b[2]: NULL;

    guint8 *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    guint8 *a1 = (a_color_chans > 1) ? a[1]: NULL;
    guint8 *a2 = (a_color_chans > 2) ? a[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = INT_MULT(INT_MULT(fade, *a2, t), *b2, s); 
                    a2++; 
                    b2++;
            case 2: *d1++ = INT_MULT(INT_MULT(fade, *a1, t), *b1, s); 
                    a1++; 
                    b1++;
            case 1: *d0++ = INT_MULT(INT_MULT(fade, *a0, t), *b0, s); 
                    a0++; 
                    b0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_B_ALPHA)
          {
              *da++ = INT_MULT(INT_MULT(fade, *aa, t), *ba, s); 
               aa++;
               ba++;
          }
      }
  }

  g_free(d);
  g_free(b);
  g_free(a);
}                                                                       
