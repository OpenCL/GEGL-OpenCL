#include "gegl-add.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglAddClass * klass);
static void init (GeglAdd * self, GeglAddClass * klass);

static GeglScanlineFunc get_scanline_func(GeglBinary * binary, GeglColorSpace space, GeglChannelDataType type);

static void a_add_b_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);

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

      type = g_type_register_static (GEGL_TYPE_BINARY, 
                                     "GeglAdd", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglAddClass * klass)
{
  GeglBinaryClass *blend_class = GEGL_BINARY_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  blend_class->get_scanline_func = get_scanline_func;
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  a_add_b_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglBinary * binary,
                  GeglColorSpace space,
                  GeglChannelDataType type)
{
  return scanline_funcs[type];
}

static void                                                            
a_add_b_float (GeglFilter * filter,              
               GeglTileIterator ** iters,        
               gint width)                       
{                                                                       
  GeglBinary * binary = GEGL_BINARY(filter);

  gfloat **d = (gfloat**)gegl_tile_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_tile_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_tile_iterator_get_num_colors(iters[0]);

  gfloat **b = (gfloat**)gegl_tile_iterator_color_channels(iters[1]);
  gfloat *ba = (gfloat*)gegl_tile_iterator_alpha_channel(iters[1]);
  gint b_color_chans = gegl_tile_iterator_get_num_colors(iters[1]);

  gfloat **a = (gfloat**)gegl_tile_iterator_color_channels(iters[2]);
  gfloat *aa = (gfloat*)gegl_tile_iterator_alpha_channel(iters[2]);
  gint a_color_chans = gegl_tile_iterator_get_num_colors(iters[2]);

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
            case 3: *d2++ = *a2++ + fade * *b2++; 
            case 2: *d1++ = *a1++ + fade * *b1++;
            case 1: *d0++ = *a0++ + fade * *b0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_B_ALPHA)
          {
              *da++ = CLAMP(*aa + fade * *ba, 0, 1);
               aa++;
               ba++;
          }
      }
  }

  g_free(d);
  g_free(b);
  g_free(a);
}                                                                       
