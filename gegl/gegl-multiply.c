#include "gegl-multiply.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglMultiplyClass * klass);
static void init (GeglMultiply * self, GeglMultiplyClass * klass);

static GeglScanlineFunc get_scanline_func(GeglComp * comp, GeglColorSpaceType space, GeglChannelSpaceType type);

static void fg_multiply_bg_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_multiply_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMultiplyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMultiply),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_BLEND, 
                                     "GeglMultiply", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMultiplyClass * klass)
{
  GeglCompClass *comp_class = GEGL_COMP_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  comp_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglMultiply * self, 
      GeglMultiplyClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  fg_multiply_bg_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglComp * comp,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}


static void                                                            
fg_multiply_bg_float (GeglFilter * filter,              
                      GeglImageIterator ** iters,        
                      gint width)                       
{                                                                       
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gfloat **b = (gfloat**)gegl_image_iterator_color_channels(iters[1]);
  gfloat *ba = (gfloat*)gegl_image_iterator_alpha_channel(iters[1]);
  gint b_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  gfloat **f = (gfloat**)gegl_image_iterator_color_channels(iters[2]);
  gfloat * fa = (gfloat*)gegl_image_iterator_alpha_channel(iters[2]);
  gint f_color_chans = gegl_image_iterator_get_num_colors(iters[2]);

  gint alpha_mask = 0x0;

  if(ba) 
    alpha_mask |= GEGL_BG_ALPHA; 
  if(fa)
    alpha_mask |= GEGL_FG_ALPHA; 

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    gfloat *b0 = (b_color_chans > 0) ? b[0]: NULL;   
    gfloat *b1 = (b_color_chans > 1) ? b[1]: NULL;
    gfloat *b2 = (b_color_chans > 2) ? b[2]: NULL;

    gfloat *f0 = (f_color_chans > 0) ? f[0]: NULL;   
    gfloat *f1 = (f_color_chans > 1) ? f[1]: NULL;
    gfloat *f2 = (f_color_chans > 2) ? f[2]: NULL;

    switch(alpha_mask)
      {
      case GEGL_NO_ALPHA:
      /* 
          (1-ba)*f + (1-fa)*b + ba*f 
          = (1-fa)*b + f

          fa->fa*op
          f -> f * op

          (1-op)b + op*f

          b + (f-b)*op
       */ 

      /* 
          (1-ba)*f + (1-fa)*b + f*b 
          (1 - op)*b + op*f*b  
       */ 
        switch(d_color_chans)
          {
            case 3: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = *f0++ * *b0++;
                  *d1++ = *f1++ * *b1++;
                  *d2++ = *f2++ * *b2++; 
                }
              break;
            case 2: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = *f0++ * *b0++;
                  *d1++ = *f1++ * *b1++;
                }
              break;
            case 1: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = *f0++ * *b0++;
                }
              break;
          }
        break;
      case GEGL_FG_ALPHA:
          {
            gfloat a;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa++;                                              
                      *d0++ = (a  + *f0++) * *b0++;   
                      *d1++ = (a  + *f1++) * *b1++;   
                      *d2++ = (a  + *f2++) * *b2++;   
                      *da++ = 1.0; 
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa++;                                              
                      *d0++ = (a  + *f0++) * *b0++;   
                      *d1++ = (a  + *f1++) * *b1++;   
                      *da++ = 1.0; 
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa++;                                              
                      *d0++ = (a  + *f0++) * *b0++;   
                      *da++ = 1.0; 
                    }
                  break;
              }
          }
        break;
      case GEGL_BG_ALPHA:
          {
            gfloat b;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      b = 1.0 - *ba++;                                              

                      *d0++ = (b  + *b0++) * *f0++;   
                      *d1++ = (b  + *b1++) * *f1++;   
                      *d2++ = (b  + *b2++) * *f2++;   

                      *da++ = 1.0; 
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      b = 1.0 - *ba++;                                              

                      *d0++ = (b  + *b0++) * *f0++;   
                      *d1++ = (b  + *b1++) * *f1++;   

                      *da++ = 1.0; 
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      b = 1.0 - *ba++;                                              

                      *d0++ = (b  + *b0++) * *f0++;   

                      *da++ = 1.0; 
                    }
                  break;
              }
          }
        break;
      case GEGL_FG_BG_ALPHA:
          {
            gfloat a;                                              
            gfloat b;                                               
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      b = 1.0 - *ba;                                               

                      /* 
                          (1-ba)*f + (1-fa)*b + f*b 
                          (1-ba)*op*f + (1 - op*fa)*b + op*f*b  
                       */ 

                      *d0++ = a * *b0 + b * *f0 + *b0 * *f0; b0++; f0++;
                      *d1++ = a * *b1 + b * *f1 + *b1 * *f1; b1++; f1++;
                      *d2++ = a * *b2 + b * *f2 + *b2 * *f2; b2++; f2++;
                      *da++ = *fa + *ba - *ba * *fa; ba++; fa++;
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      b = 1.0 - *ba;                                               

                      *d0++ = a * *b0 + b * *f0 + *b0 * *f0; b0++; f0++;
                      *d1++ = a * *b1 + b * *f1 + *b1 * *f1; b1++; f1++;
                      *da++ = *fa + *ba - *ba * *fa; ba++; fa++;
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      b = 1.0 - *ba;                                               

                      *d0++ = a * *b0 + b * *f0 + *b0 * *f0; b0++; f0++;
                      *da++ = *fa + *ba - *ba * *fa; ba++; fa++;
                    }
                  break;
              }
          }
        break;
      }
  }

  g_free(d);
  g_free(b);
  g_free(f);
}                                                                       
