#include "gegl-fade.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-scalar-data.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
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

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void fade_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);
static void fade_uint8 (GeglFilter * filter, GeglImageIterator ** iters, gint width);

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
                                                       "The multiplier for fade",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_CONSTRUCT |
                                                       G_PARAM_READWRITE));
}

static void 
init (GeglFade * self, 
      GeglFadeClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "multiplier");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglFade *self = GEGL_FADE(gobject);
  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "multiplier");
        g_value_set_float(value, g_value_get_float(data_value));  
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
  GeglFade *self = GEGL_FADE(gobject);
  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      gegl_op_set_input_data_value(GEGL_OP(self), "multiplier", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  fade_uint8, 
  fade_float, 
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
fade_float (GeglFilter * filter,              
            GeglImageIterator ** iters,        
            gint width)                       
{                                                                       
  GeglFade * self = GEGL_FADE(filter);

  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(iters[1]);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(self), "multiplier"); 
  gfloat multiplier = g_value_get_float(value);

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

static void                                                            
fade_uint8 (GeglFilter * filter,              
            GeglImageIterator ** iters,        
            gint width)                       
{                                                                       
  GeglFade * self = GEGL_FADE(filter);

  guint8 **d = (guint8**)gegl_image_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  guint8 **a = (guint8**)gegl_image_iterator_color_channels(iters[1]);
  guint8 *aa = (guint8*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  GValue *value = gegl_op_get_input_data_value(GEGL_OP(self), "multiplier"); 
  gfloat multiplier = g_value_get_float(value);

  gint alpha_mask = 0x0;

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    guint8 *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    guint8 *a1 = (a_color_chans > 1) ? a[1]: NULL;
    guint8 *a2 = (a_color_chans > 2) ? a[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = CLAMP((gint)(multiplier * *a2 + .5), 0, 255);
                    a2++;
            case 2: *d1++ = CLAMP((gint)(multiplier * *a1 + .5), 0, 255);
                    a1++;
            case 1: *d0++ = CLAMP((gint)(multiplier * *a0 + .5), 0, 255);
                    a0++;
          }

        if(alpha_mask == GEGL_A_ALPHA)
          {
              *da++ = CLAMP((gint)(multiplier * *aa + .5), 0, 255);
              aa++;
          }
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
