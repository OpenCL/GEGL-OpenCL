#include "gegl-darken.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglDarkenClass * klass);
static void init (GeglDarken * self, GeglDarkenClass * klass);

static GeglScanlineFunc get_scanline_func(GeglComp * comp, GeglColorSpaceType space, GeglChannelSpaceType type);

static void fg_darken_bg_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_darken_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDarkenClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDarken),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_BLEND, 
                                     "GeglDarken", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglDarkenClass * klass)
{
  GeglCompClass *comp_class = GEGL_COMP_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  comp_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglDarken * self, 
      GeglDarkenClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  fg_darken_bg_float, 
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
fg_darken_bg_float (GeglFilter * filter,              
                    GeglScanlineProcessor *processor,
                    gint width)                       
{                                                                       
  GeglImageIterator *dest = 
    gegl_scanline_processor_lookup_iterator(processor, "dest");
  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(dest);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(dest);
  gint d_color_chans = gegl_image_iterator_get_num_colors(dest);

  GeglImageIterator *background = 
    gegl_scanline_processor_lookup_iterator(processor, "background");
  gfloat **b = (gfloat**)gegl_image_iterator_color_channels(background);
  gfloat *ba = (gfloat*)gegl_image_iterator_alpha_channel(background);
  gint b_color_chans = gegl_image_iterator_get_num_colors(background);

  GeglImageIterator *foreground = 
    gegl_scanline_processor_lookup_iterator(processor, "foreground");
  gfloat **f = (gfloat**)gegl_image_iterator_color_channels(foreground);
  gfloat * fa = (gfloat*)gegl_image_iterator_alpha_channel(foreground);
  gint f_color_chans = gegl_image_iterator_get_num_colors(foreground);

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
        switch(d_color_chans)
          {
            case 3: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = MIN(*f0 , *b0);
                  *d1++ = MIN(*f1 , *b1);
                  *d2++ = MIN(*f2 , *b2);
                  
                  f0++; f1++; f2++;
                  b0++; b1++; b2++;
                }
              break;
            case 2: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = MIN(*f0 , *b0);
                  *d1++ = MIN(*f1 , *b1);
                 
                  f0++; f1++; 
                  b0++; b1++;
                }
              break;
            case 1: 
              while(width--)                                                        
                {                                                                   
                  *d0++ = MIN(*f0 , *b0);
                  f0++; 
                  b0++;
                }
              break;
          }
        break;
      case GEGL_FG_ALPHA:
        g_warning("Case not implemented yet\n");
        break;
      case GEGL_BG_ALPHA:
        g_warning("Case not implemented yet\n");
        break;
      case GEGL_FG_BG_ALPHA:
          {
            /*
               Darken:
               foreground (f,fa) 
               background (b,ba) 
               result (c,ca)

               c = min(f*ba, b*fa) + f*(1-ba) + b*(1-fa) 
               ca = fa + ba - fa*ba 
            */

            gfloat inv_fa;                                              
            gfloat inv_ba;                                               
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      inv_fa = 1.0 - *fa;                                              
                      inv_ba = 1.0 - *ba;                                               

                      *d0++ = MIN(*f0 * *ba, *b0 * *fa) + *f0 * inv_ba + *b0 * inv_fa;
                      *d1++ = MIN(*f1 * *ba, *b1 * *fa) + *f1 * inv_ba + *b1 * inv_fa;
                      *d2++ = MIN(*f2 * *ba, *b2 * *fa) + *f2 * inv_ba + *b2 * inv_fa;
                      *da++ = *fa + *ba - *ba * *fa; 

                      f0++; f1++; f2++; fa++; 
                      b0++; b1++; b2++; ba++;
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      inv_fa = 1.0 - *fa;                                              
                      inv_ba = 1.0 - *ba;                                               

                      *d0++ = MIN(*f0 * *ba, *b0 * *fa) + *f0 * inv_ba + *b0 * inv_fa;
                      *d1++ = MIN(*f1 * *ba, *b1 * *fa) + *f1 * inv_ba + *b1 * inv_fa;
                      *da++ = *fa + *ba - *ba * *fa; 

                      f0++; f1++; fa++;
                      b0++; b1++; ba++;
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      inv_fa = 1.0 - *fa;                                              
                      inv_ba = 1.0 - *ba;                                               

                      *d0++ = MIN(*f0 * *ba, *b0 * *fa) + *f0 * inv_ba + *b0 * inv_fa;
                      *da++ = *fa + *ba - *ba * *fa; 

                      f0++; fa++;
                      b0++; ba++;
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
