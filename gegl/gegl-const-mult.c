#include "gegl-const-mult.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_MULT0,
  PROP_MULT1,
  PROP_MULT2,
  PROP_MULT3,
  PROP_MULT4,
  PROP_LAST 
};

static void class_init (GeglConstMultClass * klass);
static void init (GeglConstMult * self, GeglConstMultClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpace space, GeglChannelDataType type);

static void const_mult_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_const_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglConstMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglConstMult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglConstMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglConstMultClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  unary_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_MULT0,
                                   g_param_spec_float ("mult0",
                                                       "Mult0",
                                                       "Multiplier for 0th channel.",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_MULT1,
                                   g_param_spec_float ("mult1",
                                                       "Mult1",
                                                       "Multiplier for 1st channel.",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_MULT2,
                                   g_param_spec_float ("mult2",
                                                       "Mult2",
                                                       "Multiplier for 2nd channel.",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_MULT3,
                                   g_param_spec_float ("mult3",
                                                       "Mult3",
                                                       "Multiplier for 3rd channel.",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_MULT4,
                                   g_param_spec_float ("mult4",
                                                       "Mult4",
                                                       "Multiplier for 4th channel.",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

}

static void 
init (GeglConstMult * self, 
      GeglConstMultClass * klass)
{
  self->mult0 = 1.0;
  self->mult1 = 1.0;
  self->mult2 = 1.0;
  self->mult3 = 1.0;
  self->mult4 = 1.0;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglConstMult *const_mult = GEGL_CONST_MULT(gobject);
  switch (prop_id)
  {
    case PROP_MULT0:
      g_value_set_float(value, const_mult->mult0);  
      break;
    case PROP_MULT1:
      g_value_set_float(value, const_mult->mult1);  
      break;
    case PROP_MULT2:
      g_value_set_float(value, const_mult->mult2);  
      break;
    case PROP_MULT3:
      g_value_set_float(value, const_mult->mult3);  
      break;
    case PROP_MULT4:
      g_value_set_float(value, const_mult->mult4);  
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
  GeglConstMult *const_mult = GEGL_CONST_MULT(gobject);
  switch (prop_id)
  {
    case PROP_MULT0:
      const_mult->mult0 = g_value_get_float(value);  
      break;
    case PROP_MULT1:
      const_mult->mult1 = g_value_get_float(value);  
      break;
    case PROP_MULT2:
      const_mult->mult2 = g_value_get_float(value);  
      break;
    case PROP_MULT3:
      const_mult->mult3 = g_value_get_float(value);  
      break;
    case PROP_MULT4:
      const_mult->mult4 = g_value_get_float(value);  
      break;
    default:
      break;
  }
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  const_mult_float, 
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
const_mult_float (GeglFilter * filter,              
            GeglTileIterator ** iters,        
            gint width)                       
{                                                                       
  GeglConstMult * self = GEGL_CONST_MULT(filter);
  gfloat mult0 = self->mult0;
  gfloat mult1 = self->mult1;
  gfloat mult2 = self->mult2;
  gfloat mult3 = self->mult3;

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

    /* This needs to actually match multipliers to channels returned */
    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = mult2 * *a2++;
            case 2: *d1++ = mult1 * *a1++;
            case 1: *d0++ = mult0 * *a0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_ALPHA)
          {
              *da++ = mult3 * *aa++;
          }
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
