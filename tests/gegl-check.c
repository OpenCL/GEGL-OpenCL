#include "gegl-check.h"
#include "gegl.h"

enum
{
  PROP_0, 
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_X,
  PROP_Y,
  PROP_IMAGE_BUFFER,
  PROP_LAST 
};

static void class_init (GeglCheckClass * klass);
static void init (GeglCheck * self, GeglCheckClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void process (GeglFilter * filter, GList  * output_params, GList * input_params);

static void check_float(GeglCheck *self, GeglImageBufferIterator *iter, gint x);
static void check_uint8(GeglCheck *self, GeglImageBufferIterator *iter, gint x);

static gpointer parent_class = NULL;

GType
gegl_check_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCheckClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCheck),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglCheck", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCheckClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->process = process;

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
                                   gegl_param_spec_pixel_rgb_float ("pixel-rgb-float",
                                                                    "Pixel-Rgb-Float",
                                                                    "The pixel as float",
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    -G_MAXFLOAT, G_MAXFLOAT,
                                                                    0.0, 0.0, 0.0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
                                   gegl_param_spec_pixel_rgb_uint8 ("pixel-rgb-uint8",
                                                                    "Pixel-Rgb-Uint8",
                                                                    "The pixel as uint8",
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 255,
                                                                    0, 0, 0,
                                                                    G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x",
                                                      "X",
                                                      "x to check pixel of",
                                                      G_MININT,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                      "Y",
                                                      "y to check pixel of",
                                                      G_MININT,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE_BUFFER,
                                   g_param_spec_object ("image_buffer",
                                                        "ImageBuffer",
                                                        "The image_buffer",
                                                         GEGL_TYPE_IMAGE_BUFFER,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglCheck * self, 
      GeglCheckClass * klass)
{
  self->x = 0;
  self->y = 0;
  self->image_buffer = NULL;

  self->pixel = g_new0(GValue, 1); 
  g_value_init(self->pixel, GEGL_TYPE_PIXEL_RGB_FLOAT);
  g_value_set_pixel_rgb_float(self->pixel, 0.0, 0.0, 0.0);
}

static void
finalize(GObject *gobject)
{
  GeglCheck *self = GEGL_CHECK (gobject);

  /* Delete the reference to the pixel*/
  g_free(self->pixel);

  if(self->image_buffer)
   g_object_unref(self->image_buffer);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglCheck *self = GEGL_CHECK(gobject);
  switch (prop_id)
  {
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      gegl_check_get_pixel(self, value); 
      break;
    case PROP_IMAGE_BUFFER:
      g_value_set_object (value, gegl_check_get_image_buffer(self));
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
  GeglCheck *self = GEGL_CHECK(gobject);
  switch (prop_id)
  {
    case PROP_X:
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      gegl_check_set_pixel(self, (GValue*)value); 
      break;
    case PROP_IMAGE_BUFFER:
      gegl_check_set_image_buffer(self, (GeglImageBuffer*)g_value_get_object(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

gboolean 
gegl_check_get_success(GeglCheck *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_CHECK (self), FALSE);

  return self->success;
}

void
gegl_check_get_pixel (GeglCheck * self,
                      GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_CHECK (self));
  g_return_if_fail (g_value_type_compatible(G_VALUE_TYPE(self->pixel), G_VALUE_TYPE(pixel)));

  /* Dont transform the return, just copy */
  g_value_copy(self->pixel, pixel); 
}

void
gegl_check_set_pixel (GeglCheck * self, 
                      GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_CHECK (self));

  g_value_unset(self->pixel);
  g_value_init(self->pixel, G_VALUE_TYPE(pixel));
  g_value_copy(pixel, self->pixel); 
}

void
gegl_check_set_image_buffer (GeglCheck * self,
                           GeglImageBuffer *image_buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_CHECK (self));

  if(image_buffer)
   g_object_ref(image_buffer);

  if(self->image_buffer)
   g_object_unref(self->image_buffer);

  self->image_buffer = image_buffer;
}

GeglImageBuffer *
gegl_check_get_image_buffer (GeglCheck * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_CHECK (self), NULL);

  return self->image_buffer;
}

static void 
process (GeglFilter * filter, 
         GList * output_params,
         GList * input_params)
{
  GeglCheck *self =  GEGL_CHECK(filter);
  GeglColorModel *color_model = gegl_image_buffer_get_color_model(self->image_buffer);
  GeglColorModel *pixel_color_model = g_value_pixel_get_color_model(self->pixel);
  /* Shouldnt be accessing tile directly*/
  GeglTile * tile = gegl_image_buffer_get_tile(self->image_buffer);
  GeglDataSpace *data_space = gegl_color_model_data_space(color_model);
  GeglRect rect;
  GeglImageBufferIterator *iter;
  gint x;

  if(color_model != pixel_color_model)
    {
      self->success = FALSE;
      LOG_DIRECT("Color models dont match. Expected: %s Found %s", 
                 gegl_color_model_name(pixel_color_model), 
                 gegl_color_model_name(color_model));
      return;
    }
    
  gegl_tile_get_area(tile, &rect);
  iter = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                       "image_buffer", self->image_buffer,
                       "area", &rect,
                       NULL);  

  gegl_image_buffer_iterator_set_scanline(iter, self->y);
  x = self->x - rect.x;

  if(data_space == gegl_data_space_instance("float"))
    {
      check_float(self, iter, self->x - rect.x);
    }
  else if(data_space == gegl_data_space_instance("uint8"))
    {
      check_uint8(self, iter, self->x - rect.x);
    }
  else 
    g_warning("Cant find check for data space");

  g_object_unref(iter);
}

static void
check_float(GeglCheck *self,
            GeglImageBufferIterator *iter,
            gint x)
{
  gfloat *data = (gfloat*)g_value_pixel_get_data(self->pixel);
  gfloat **a = (gfloat**)gegl_image_buffer_iterator_color_channels(iter);
  gfloat *aa = (gfloat*)gegl_image_buffer_iterator_alpha_channel(iter);
  gint a_color_chans = gegl_image_buffer_iterator_get_num_colors(iter);
  gfloat expected;
  gfloat found;

  self->success = TRUE;

  /*LOG_DIRECT("x y %d %d", self->x, self->y);*/

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
      case 0:  
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
check_uint8(GeglCheck *self,
            GeglImageBufferIterator *iter,
            gint x)
{
  guint8 *data = (guint8*)g_value_pixel_get_data(self->pixel);
  guint8 **a = (guint8**)gegl_image_buffer_iterator_color_channels(iter);
  guint8 *aa = (guint8*)gegl_image_buffer_iterator_alpha_channel(iter);
  gint a_color_chans = gegl_image_buffer_iterator_get_num_colors(iter);
  guint8 expected;
  guint8 found;

  self->success = TRUE;

  /* LOG_DIRECT("x y %d %d", self->x, self->y);*/
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
      case 0:  
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
