#include "gegl-add.h"
#include "gegl-image-iterator.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-pixel-data.h"

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

static void validate_inputs  (GeglFilter *filter, GList *data);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void add_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);
static void add_uint8 (GeglFilter * filter, GeglImageIterator ** iters, gint width);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  GParamSpec *generic_property;
  GParamSpec *property;

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->validate_inputs = validate_inputs;

  unary_class->get_scanline_func = get_scanline_func;

  /* op data properties */
  gegl_op_class_install_data_input_property(op_class, 
                              gegl_param_spec_pixel("constant", 
                                                    "Constant",
                                                    "Constant pixel",
                                                     G_PARAM_PRIVATE));

  /* gobject properties */
  generic_property = gegl_op_class_find_data_input_property(op_class, "constant"); 

  property = gegl_param_spec_pixel_rgb_float ("constant-rgb-float",
                                              "ConstantRgbFloat",
                                              "The rgb float constant",
                                              -G_MAXFLOAT, G_MAXFLOAT,
                                              -G_MAXFLOAT, G_MAXFLOAT,
                                              -G_MAXFLOAT, G_MAXFLOAT,
                                              0.0, 0.0, 0.0,
                                              G_PARAM_READWRITE);
  g_param_spec_set_qdata(generic_property, g_quark_from_string("rgb-float"), property);
  g_object_class_install_property (gobject_class, PROP_CONSTANT_FLOAT, property);

  property = gegl_param_spec_pixel_rgb_uint8 ("constant-rgb-uint8",
                                              "ConstantRgbUint8",
                                              "The rgb uint8 constant",
                                              0, 255,
                                              0, 255,
                                              0, 255,
                                              0, 0, 0,
                                              G_PARAM_READWRITE);
  g_param_spec_set_qdata(generic_property, g_quark_from_string("rgb-uint8"), property);
  g_object_class_install_property (gobject_class, PROP_CONSTANT_UINT8, property);
}

static void 
init (GeglAdd * self, 
      GeglAddClass * klass)
{
  /* Add the constant input. */
  gegl_op_append_input(GEGL_OP(self), GEGL_TYPE_PIXEL_DATA, "constant");


  /* Default constant is float type */
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

static void 
validate_inputs  (GeglFilter *filter, 
                  GList *data_inputs)
{
  GeglAdd *self = GEGL_ADD(filter);
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, data_inputs);

  {
    /* This data input will be the converted input */
    GeglData * data_input = gegl_op_get_data_input(GEGL_OP(filter), 1);
    GeglData * data_output = gegl_op_get_data_output(GEGL_OP(filter), 0);

    GeglColorModel *color_model = 
      gegl_color_data_get_color_model(GEGL_COLOR_DATA(data_output));

    /* Set the desired color model of the converted input */
    gegl_color_data_set_color_model(GEGL_COLOR_DATA(data_input), color_model);

    /* Copy/Convert input data to the converted input. */
    g_value_init(data_input->value, G_VALUE_TYPE(self->constant));
    g_value_copy(self->constant, data_input->value);
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
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
add_float (GeglFilter * filter,              
           GeglImageIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  gfloat* data = (gfloat*)g_value_pixel_get_data(self->constant);

  gfloat **d = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(iters[1]);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

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
           GeglImageIterator ** iters,        
           gint width)                       
{                                                                       
  GeglAdd * self = GEGL_ADD(filter);

  guint8* data = (guint8*)g_value_pixel_get_data(self->constant);

  guint8 **d = (guint8**)gegl_image_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  guint8 **a = (guint8**)gegl_image_iterator_color_channels(iters[1]);
  guint8 *aa = (guint8*)gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

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
