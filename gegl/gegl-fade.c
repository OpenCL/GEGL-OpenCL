#include "gegl-fade.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_MULTIPLIER,
  PROP_LAST 
};

static void class_init (GeglFadeClass * klass);
static void init (GeglFade * self, GeglFadeClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpace space, GeglChannelDataType type);

static void fade_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_fade_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFadeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFade),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglFade", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFadeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  unary_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_MULTIPLIER,
                                   g_param_spec_float ("multiplier",
                                                       "Multiplier",
                                                       "Multiplier for image.",
                                                       0.0, 
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));
}

static void 
init (GeglFade * self, 
      GeglFadeClass * klass)
{
  self->multiplier = 1.0;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglFade *fade = GEGL_FADE(gobject);
  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      g_value_set_float(value, gegl_fade_get_multiplier(fade));  
      break;
    default:
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglFade *fade = GEGL_FADE(gobject);
  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      gegl_fade_set_multiplier(fade, g_value_get_float(value));  
      break;
    default:
      break;
  }
}

/**
 * gegl_fade_get_multiplier:
 * @self: a #GeglFade.
 *
 * Gets the multiplier factor.
 *
 * Returns: the multiplier factor. 
 **/
gfloat
gegl_fade_get_multiplier (GeglFade * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_FADE (self), FALSE);

  return self->multiplier;
}


/**
 * gegl_fade_set_multiplier:
 * @self: a #GeglFade.
 * @multiplier: the multiplier. 
 *
 * Sets the multiplier factor. 
 *
 **/
void 
gegl_fade_set_multiplier (GeglFade * self, 
                          gfloat multiplier)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FADE (self));

  self->multiplier = multiplier;
}


/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  fade_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglUnary * unary,
                  GeglColorSpace space,
                  GeglChannelDataType type)
{
  return scanline_funcs[type];
}

static void                                                            
fade_float (GeglFilter * filter,              
            GeglTileIterator ** iters,        
            gint width)                       
{                                                                       
  GeglFade * self = GEGL_FADE(filter);
  gfloat multiplier = self->multiplier;

  gfloat **d = (gfloat**)gegl_tile_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_tile_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_tile_iterator_get_num_colors(iters[0]);

  gfloat **a = (gfloat**)gegl_tile_iterator_color_channels(iters[1]);
  gfloat *aa = (gfloat*)gegl_tile_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_tile_iterator_get_num_colors(iters[1]);

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

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = multiplier * *a2++;
            case 2: *d1++ = multiplier * *a1++;
            case 1: *d0++ = multiplier * *a0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_ALPHA)
          {
              *da++ = multiplier * *aa++;
          }
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
