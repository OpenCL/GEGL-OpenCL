#include "gegl-premult.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglPremultClass * klass);
static void init (GeglPremult * self, GeglPremultClass * klass);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void premult_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

static gpointer parent_class = NULL;

GType
gegl_premult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPremultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPremult),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglPremult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPremultClass * klass)
{
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  unary_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglPremult * self, 
      GeglPremultClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  premult_float, 
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
premult_float (GeglFilter * filter,              
               GeglScanlineProcessor *processor,
               gint width)                       
{                                                                       
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
          while(width--)                                                        
            {                                                                   
              *d0++ = *aa * *a0++;
              *d1++ = *aa * *a1++;
              *d2++ = *aa * *a2++;
              *da++ = *aa++;
            }
          break;
        case 2: 
          while(width--)                                                        
            {                                                                   
              *d0++ = *aa * *a0++;
              *d1++ = *aa * *a1++;
              *da++ = *aa++;
            }
          break;
        case 1: 
          while(width--)                                                        
            {                                                                   
              *d0++ = *aa * *a0++;
              *da++ = *aa++;
            }
          break;
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
