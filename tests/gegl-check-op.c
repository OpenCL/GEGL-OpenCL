#include "gegl-check-op.h"
#include "gegl-op.h"
#include "gegl.h"

enum
{
  PROP_0, 
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGBA_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_PIXEL_RGBA_UINT8,
  PROP_X,
  PROP_Y,
  PROP_IMAGE_OP,
  PROP_LAST 
};

static void class_init (GeglCheckOpClass * klass);
static void init (GeglCheckOp * self, GeglCheckOpClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void validate_inputs(GeglFilter *filter, GArray *collected_data);

static void process (GeglFilter * filter);

static void check_op_float(GeglCheckOp *self, GeglImageIterator *iter, gint x);
static void check_op_uint8(GeglCheckOp *self, GeglImageIterator *iter, gint x);

static gpointer parent_class = NULL;

GType
gegl_check_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCheckOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCheckOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglCheckOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCheckOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->process = process;
  filter_class->validate_inputs = validate_inputs;

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
                                   gegl_param_spec_pixel_rgb_float ("pixel-rgb-float",
                                                                    "Pixel-Rgb-Float",
                                                                    "rgb float pixel",
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    0.0, 0.0, 0.0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGBA_FLOAT,
                                   gegl_param_spec_pixel_rgba_float ("pixel-rgba-float",
                                                                    "Pixel-Rgba-Float",
                                                                    "rgba float pixel",
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    0.0,1.0,
                                                                    0.0, 0.0, 0.0, 0.0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
                                   gegl_param_spec_pixel_rgb_uint8 ("pixel-rgb-uint8",
                                                                    "Pixel-Rgb-Uint8",
                                                                    "rgb uint8 pixel",
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 0, 0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGBA_UINT8,
                                   gegl_param_spec_pixel_rgba_uint8 ("pixel-rgba-uint8",
                                                                     "Pixel-Rgba-Uint8",
                                                                     "rgba uint8 pixel",
                                                                     0, 255,
                                                                     0, 255,
                                                                     0, 255,
                                                                     0, 255,
                                                                     0, 0, 0, 0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x",
                                                      "X",
                                                      "x to check_op pixel of",
                                                      G_MININT,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                      "Y",
                                                      "y to check_op pixel of",
                                                      G_MININT,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE_OP,
                                   g_param_spec_object ("image-op",
                                                        "ImageOp",
                                                        "The image op",
                                                         GEGL_TYPE_IMAGE_OP,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglCheckOp * self, 
      GeglCheckOpClass * klass)
{
  self->image_op = NULL;

  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_PIXEL_DATA, "pixel");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "x");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "y");
}

static void
finalize(GObject *gobject)
{
  GeglCheckOp *self = GEGL_CHECK_OP(gobject);

  if(self->image_op)
   g_object_unref(self->image_op);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglCheckOp *self = GEGL_CHECK_OP(gobject);
  switch (prop_id)
  {
    case PROP_X:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "x");
        g_value_set_int(value, g_value_get_int(data_value));  
      }
      break;
    case PROP_Y:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "y");
        g_value_set_int(value, g_value_get_int(data_value));  
      }
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGBA_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
    case PROP_PIXEL_RGBA_UINT8:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
      break;
    case PROP_IMAGE_OP:
      g_value_set_object (value, (GObject*)(self->image_op));
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
  GeglCheckOp *self = GEGL_CHECK_OP(gobject);
  switch (prop_id)
  {
    case PROP_X:
      gegl_op_set_input_data_value(GEGL_OP(self), "x", value);
      break;
    case PROP_Y:
      gegl_op_set_input_data_value(GEGL_OP(self), "y", value);
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGBA_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
    case PROP_PIXEL_RGBA_UINT8:
      gegl_op_set_input_data_value(GEGL_OP(self), "pixel", value);
      break;
    case PROP_IMAGE_OP:
      self->image_op = (GeglImageOp*)g_value_get_object(value);
      if(self->image_op)
        g_object_ref(self->image_op);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

gboolean 
gegl_check_op_get_success(GeglCheckOp *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_CHECK_OP (self), FALSE);

  return self->success;
}

static void
validate_inputs(GeglFilter *filter,
                      GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);
#if 0 
  /* Check that the pixel is the same color model as image op */
  {
    GeglData *pixel_data = 
      gegl_op_get_input_data(GEGL_OP(filter), "pixel"); 
    GeglData *image_data = 
      gegl_op_get_output_data(GEGL_OP(self->image_op), "dest"); 

    GeglColorModel *pixel_color_model = 
      gegl_color_data_get_color_model(GEGL_COLOR_DATA(pixel_data));

    GeglColorModel *image_color_model = 
      gegl_color_data_get_color_model(GEGL_COLOR_DATA(image_data));

    if(image_color_model != pixel_color_model)
      {
        gegl_log_direct("Color models dont match. Expected: %s Found %s", 
                   gegl_color_model_name(pixel_color_model), 
                   gegl_color_model_name(image_color_model));
        self->success = FALSE;
        g_error("Color models are different for pixel and image op data\n");
      }
  }
#endif
}

static void 
process (GeglFilter * filter)
{
  GeglCheckOp *self =  GEGL_CHECK_OP(filter);
  GeglRect rect;

  GValue *x_value = gegl_op_get_input_data_value(GEGL_OP(self), "x");
  GValue *y_value = gegl_op_get_input_data_value(GEGL_OP(self), "y");
  gint x = g_value_get_int(x_value); 
  gint y = g_value_get_int(y_value); 

  GeglData *image_op_data = 
        gegl_op_get_output_data(GEGL_OP(self->image_op), "dest"); 
  GValue *image_value = gegl_data_get_value(image_op_data);
  GeglImage *image = g_value_get_object(image_value);
  GeglColorModel *color_model = 
    gegl_color_data_get_color_model(GEGL_COLOR_DATA(image_op_data));
  GeglChannelSpace *channel_space = gegl_color_model_channel_space(color_model);
  GeglImageIterator *iter;

  gegl_image_data_get_rect(GEGL_IMAGE_DATA(image_op_data), &rect);

  iter = g_object_new (GEGL_TYPE_IMAGE_ITERATOR, 
                       "image", image,
                       "area", &rect,
                       NULL);  

  gegl_image_iterator_set_scanline(iter, y);

  if(channel_space == gegl_channel_space_instance("float"))
    {
      check_op_float(self, iter, x - rect.x);
    }
  else if(channel_space == gegl_channel_space_instance("uint8"))
    {
      check_op_uint8(self, iter, x - rect.x);
    }
  else 
    g_warning("Cant find check_op for data space");

  g_object_unref(iter);
}

static void
check_op_float(GeglCheckOp *self,
               GeglImageIterator *iter,
               gint x)
{
  GValue *pixel_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
  gfloat *data = (gfloat*)g_value_pixel_get_data(pixel_value);
  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(iter);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(iter);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iter);
  gfloat expected;
  gfloat found;

  self->success = TRUE;

  /*gegl_log_direct("x y %d %d", self->x, self->y);*/

  switch(a_color_chans)
    {

      case 3: 
              expected = data[2];
              found = *(a[2] + x);
              self->success &= GEGL_FLOAT_EQUAL(expected, found);
              if(!self->success)
                g_warning("Expected %f but found %f in channel %d\n", 
                           expected, found, 2);
      case 2: 
              expected = data[1];
              found = *(a[1] + x);
              self->success &= GEGL_FLOAT_EQUAL(expected, found);
              if(!self->success)
                g_warning("Expected %f but found %f in channel %d\n", 
                           expected, found, 1);
      case 1: 
              expected = data[0];
              found = *(a[0] + x);
              self->success &= GEGL_FLOAT_EQUAL(expected, found);
              if(!self->success)
                g_warning("Expected %f but found %f in channel %d\n", 
                           expected, found, 0);
    }

  if(aa)
    {
        expected = data[3];
        found = *(aa + x);
        self->success &= GEGL_FLOAT_EQUAL(expected, found);
        if(!self->success)
          g_warning("Expected %f but found %f in alpha channel %d\n", 
                     expected, found, 3);
    }

  g_free(a);
}

static void
check_op_uint8(GeglCheckOp *self,
            GeglImageIterator *iter,
            gint x)
{
  GValue *pixel_value = gegl_op_get_input_data_value(GEGL_OP(self), "pixel");
  guint8 *data = (guint8*)g_value_pixel_get_data(pixel_value);
  guint8 **a = (guint8**)gegl_image_iterator_color_channels(iter);
  guint8 *aa = (guint8*)gegl_image_iterator_alpha_channel(iter);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iter);
  guint8 expected;
  guint8 found;

  self->success = TRUE;

  /* gegl_log_direct("x y %d %d", self->x, self->y);*/
  /* g_print("addresses are: %p %p %p\n",  data[0], data[1], data[2]); */

  switch(a_color_chans)
    {
      case 3: 
              expected = data[2];
              found = *(a[2] + x);
              self->success &= (expected == found);
              if(!self->success)
                g_warning("Expected %d but found %d in channel %d\n", 
                           expected, found, 2);
      case 2: 
              expected = data[1];
              found = *(a[1] + x);
              self->success &= (expected == found);
              if(!self->success)
                g_warning("Expected %d but found %d in channel %d\n", 
                           expected, found, 1);
      case 1: 
              expected = data[0];
              found = *(a[0] + x);
              self->success &= (expected == found);
              if(!self->success)
                g_warning("Expected %d but found %d in channel %d\n", 
                           expected, found, 0);
    }

  if(aa)
    {
        expected = data[3];
        found = *(aa + x);
        self->success &= (expected == found);
        if(!self->success)
          g_warning("Expected %d but found %d in alpha channel %d\n", 
                     expected, found, 3);
    }

  g_free(a);
}
