#include "gegl-normal.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglNormalClass * klass);
static void init (GeglNormal * self, GeglNormalClass * klass);

static GeglScanlineFunc get_scanline_func(GeglComp * comp, GeglColorSpaceType space, GeglChannelSpaceType type);

static void fg_normal_bg_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_normal_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglNormalClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglNormal),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_BLEND, 
                                     "GeglNormal", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglNormalClass * klass)
{
  GeglCompClass *comp_class = GEGL_COMP_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  comp_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglNormal * self, 
      GeglNormalClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  fg_normal_bg_float, 
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
fg_normal_bg_float (GeglFilter * filter,              
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

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(filter), "opacity"); 
  gfloat opacity = g_value_get_float(value);

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

    /* 
       For normal mode, have:

       c = (1-fa)*b + f
       ca = fa + ba - fa*ba

       and so a version weighted by opacity:

       c = (1-opacity*fa)*b + opacity*f
       ca = opacity*fa + ba - opacity*fa*ba
     */ 

    switch(alpha_mask)
      {
      case GEGL_NO_ALPHA:
          {
            /* 
              Here fa = ba = 1.0, so have
              c = (1-opacity)*b + opacity*f
              ca = opacity + 1.0 - opacity = 1.0 (we dont need this) 
            */
            gfloat a;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - opacity;                                              

                      *d0++ = a * *b0++ + opacity * *f0++;
                      *d1++ = a * *b1++ + opacity * *f1++;
                      *d2++ = a * *b2++ + opacity * *f2++;
                    }
                  break;
                case 2: 
                case 1: 
                  g_print("Case not implemented yet\n");
                  break;
              }
          }
        break;
      case GEGL_FG_ALPHA:
          {
            /* 
              Here ba = 1.0
              c = (1-opacity*fa)*b + opacity*f
              ca = opacity*fa + 1.0 - opacity*fa = 1.0 (unused)
            */
            gfloat a;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - opacity * *fa; 
                      fa++;                                              

                      *d0++ = a * *b0++ + opacity * *f0++;
                      *d1++ = a * *b1++ + opacity * *f1++;
                      *d2++ = a * *b2++ + opacity * *f2++;
                    }
                  break;
                case 2: 
                case 1: 
                  g_print("Case not implemented yet\n");
                  break;
              }
          }
        break;
      case GEGL_BG_ALPHA:
          {
            /* 
              Here fa = 1.0, so
              c = (1-opacity)*b + opacity*f
              ca = opacity + ba - opacity*ba;
            */

            gfloat a;                                              
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      a = 1.0 - opacity;                                              

                      *d0++ = a * *b0++ + opacity * *f0++; 
                      *d1++ = a * *b1++ + opacity * *f1++; 
                      *d2++ = a * *b2++ + opacity * *f2++;
                      *da++ = a * *ba++ + opacity; 
                    }
                  break;
                case 2: 
                case 1: 
                  g_print("Case not implemented yet\n");
                  break;
              }
          }
        break;
      case GEGL_FG_BG_ALPHA:
          {
            /* 
              c = (1-opacity*fa)*b + opacity*f
              ca = opacity*fa + ba - opacity*fa*ba
            */

            gfloat a;                                              
            gfloat factor;
            switch(d_color_chans)
              {
                case 3: 
                  while(width--)                                                        
                    {                                                                   
                      factor = opacity * *fa++;
                      a = 1.0 - factor;                                              

                      *d0++ = a * *b0++ + opacity * *f0++;
                      *d1++ = a * *b1++ + opacity * *f1++;
                      *d2++ = a * *b2++ + opacity * *f2++;
                      *da++ = a * *ba++ + factor;
                    }
                  break;
                case 2: 
                case 1: 
                  g_print("Case not implemented yet\n");
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
