#include "gegl-add.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-data-iterator.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_CONSTANT_FLOAT,
  PROP_CONSTANT_UINT8,
  PROP_LAST 
};

static void class_init (GeglAddClass * klass);
static void init (GeglAdd * self, GeglAddClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglDataSpaceType type);

static void add_float (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);
static void add_uint8 (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);

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

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglAdd", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglAddClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  unary_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_CONSTANT_FLOAT,
                                   gegl_param_spec_pixel_rgb_float ("constant-float",
                                                                    "ConstantFloat",
                                                                    "The float constant",
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    0.0, 0.0, 0.0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CONSTANT_UINT8,
                                   gegl_param_spec_pixel_rgb_uint8 ("constant-uint8",
                                                                    "Color-Rgb-Uint8",
                                                                    "The uint8 constant",
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 0, 0,
                                                                    G_PARAM_READWRITE));
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
  self->constant = g_new0(GValue, 1); 
  g_value_init(self->constant, GEGL_TYPE_PIXEL_RGB_FLOAT);
  g_value_set_pixel_rgb_float(self->constant, 0.0, 0.0, 0.0);
}

static void
finalize(GObject *gobject)
{
  GeglAdd *self = GEGL_ADD (gobject);

  g_free(self->constant);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglAdd *self = GEGL_ADD(gobject);
  switch (prop_id)
  {
    case PROP_CONSTANT_FLOAT:
    case PROP_CONSTANT_UINT8:
      gegl_add_get_constant(self, value); 
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
  GeglAdd *self = GEGL_ADD(gobject);
  switch (prop_id)
  {
    case PROP_CONSTANT_FLOAT:
    case PROP_CONSTANT_UINT8:
      gegl_add_set_constant(self, (GValue*)value); 
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

void
gegl_add_get_constant (GeglAdd * self,
                        GValue *constant)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_ADD (self));
  g_return_if_fail (g_value_type_transformable(G_VALUE_TYPE(self->constant), G_VALUE_TYPE(constant)));

  g_value_transform(self->constant, constant); 
}

void
gegl_add_set_constant (GeglAdd * self, 
                        GValue *constant)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_ADD (self));

  g_value_unset(self->constant);
  g_value_init(self->constant, G_VALUE_TYPE(constant));
  g_value_copy(constant, self->constant); 
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  add_uint8, 
  add_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglUnary * unary,
                  GeglColorSpaceType space,
                  GeglDataSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
add_float (GeglFilter * filter,              
           GeglImageDataIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  gfloat* data = (gfloat*)g_value_pixel_get_data(self->constant);

  gfloat **d = (gfloat**)gegl_image_data_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);

  gfloat **a = (gfloat**)gegl_image_data_iterator_color_channels(iters[1]);
  gfloat *aa = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_data_iterator_get_num_colors(iters[1]);

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
            case 3: *d2++ = *a2++ + data[2];
            case 2: *d1++ = *a1++ + data[1];
            case 1: *d0++ = *a0++ + data[0];
            case 0:        
          }

        if(alpha_mask == GEGL_A_ALPHA)
          {
              *da++ = *aa++ + data[3];
          }
      }
  }

  g_free(d);
  g_free(a);
}                                                                       

static void                                                            
add_uint8 (GeglFilter * filter,              
           GeglImageDataIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  guint8* data = (guint8*)g_value_pixel_get_data(self->constant);

  guint8 **d = (guint8**)gegl_image_data_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);

  guint8 **a = (guint8**)gegl_image_data_iterator_color_channels(iters[1]);
  guint8 *aa = (guint8*)gegl_image_data_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_data_iterator_get_num_colors(iters[1]);

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
            case 3: *d2++ = CLAMP(*a2 + data[2], 0, 255);
                    a2++;
            case 2: *d1++ = CLAMP(*a1 + data[1], 0, 255);
                    a1++;
            case 1: *d0++ = CLAMP(*a0 + data[0], 0, 255);
                    a0++;
            case 0:        
          }

        if(alpha_mask == GEGL_A_ALPHA)
          {
              *da++ = CLAMP(*aa + data[3], 0, 255);
              aa++;
          }
      }
  }

  g_free(d);
  g_free(a);
}                                                                       
