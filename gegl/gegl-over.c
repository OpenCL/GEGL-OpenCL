#include "gegl-over.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglOverClass * klass);
static void init (GeglOver * self, GeglOverClass * klass);

static GeglScanlineFunc get_scanline_func(GeglComp * comp, GeglColorSpaceType space, GeglChannelSpaceType type);

static void fg_over_bg_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_over_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglOverClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglOver),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_COMP, 
                                     "GeglOver", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglOverClass * klass)
{
  GeglCompClass *comp_class = GEGL_COMP_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  comp_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglOver * self, 
      GeglOverClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  fg_over_bg_float, 
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
fg_over_bg_float (GeglFilter * filter,              
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
          {
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                      *d1++ = *f1++;
                      *d2++ = *f2++;
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                      *d1++ = *f1++;
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                    }
                  break;
              }
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
                      *d0++ = *f0++ + *b0++ * a;
                      *d1++ = *f1++ + *b1++ * a;
                      *d2++ = *f2++ + *b2++ * a;
                      *da++ = 1.0; 
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa++;                                              
                      *d0++ = *f0++ + *b0++ * a;
                      *d1++ = *f1++ + *b1++ * a;
                      *da++ = 1.0; 
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa++;                                              
                      *d0++ = *f0++ + *b0++ * a;
                      *da++ = 1.0; 
                    }
                  break;
              }
          }
        break;
      case GEGL_BG_ALPHA:
          {
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                      *d1++ = *f1++;
                      *d2++ = *f2++;
                      *da++ = 1.0; 
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                      *d1++ = *f1++;
                      *da++ = 1.0; 
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      *d0++ = *f0++;
                      *da++ = 1.0; 
                    }
                  break;
              }
          }
        break;
      case GEGL_FG_BG_ALPHA:
          {
            /* 
               Over:
               foreground (f,fa) 
               background (b,ba) 
               result (c,ca)

               c  = f + b*(1-fa)
               ca = fa + ba*(1 - fa)
            */
            gfloat a;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      *d0++ = *f0++ + *b0++ * a;
                      *d1++ = *f1++ + *b1++ * a;
                      *d2++ = *f2++ + *b2++ * a;
                      *da++ = *fa++ + *ba++ * a; 
                    }
                  break;
                case 2: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      *d0++ = *f0++ + *b0++ * a;
                      *d1++ = *f1++ + *b1++ * a;
                      *da++ = *fa++ + *ba++ * a; 
                    }
                  break;
                case 1: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - *fa;                                              
                      *d0++ = *f0++ + *b0++ * a;
                      *da++ = *fa++ + *ba++ * a; 
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
