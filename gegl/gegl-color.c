#include "gegl-color.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-image-data-iterator.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_PIXEL_RGB_FLOAT,
  PROP_PIXEL_RGB_UINT8,
  PROP_LAST 
};

static void class_init (GeglColorClass * klass);
static void init (GeglColor * self, GeglColorClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void compute_have_rect(GeglFilter * filter, GeglRect *have_rect, GList * input_have_rects);
static GeglColorModel * compute_derived_color_model (GeglFilter * filter, GList * input_color_models);

static GeglScanlineFunc get_scanline_func(GeglNoInput * no_input, GeglColorSpaceType space, GeglDataSpaceType type);

static void color_u8 (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);
static void color_float (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);
static void color_u16 (GeglFilter * filter, GeglImageDataIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_color_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NO_INPUT, 
                                     "GeglColor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglColorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglNoInputClass *no_input_class = GEGL_NO_INPUT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  no_input_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->compute_have_rect = compute_have_rect;
  filter_class->compute_derived_color_model = compute_derived_color_model;

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                      "Width",
                                                      "GeglColor width",
                                                      0,
                                                      G_MAXINT,
                                                      GEGL_DEFAULT_WIDTH,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                      "Height",
                                                      "GeglColor height",
                                                      0,
                                                      G_MAXINT,
                                                      GEGL_DEFAULT_HEIGHT,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_FLOAT,
                                   gegl_param_spec_rgb_float ("pixel-rgb-float",
                                                              "Pixel-Rgb-Float",
                                                              "The rgb-float pixel",
                                                              0.0, 1.0,
                                                              0.0, 1.0,
                                                              0.0, 1.0,
                                                              0.0, 0.0, 0.0,
                                                              G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PIXEL_RGB_UINT8,
                                   gegl_param_spec_rgb_uint8 ("pixel-rgb-uint8",
                                                              "Pixel-Rgb-Uint8",
                                                              "The rgb-uint8 pixel",
                                                              0, 255,
                                                              0, 255,
                                                              0, 255,
                                                              0, 0, 0,
                                                              G_PARAM_READWRITE));
}

static void 
init (GeglColor * self, 
      GeglColorClass * klass)
{
  self->pixel = g_new0(GValue, 1); 
  g_value_init(self->pixel, GEGL_TYPE_RGB_FLOAT);
  g_value_set_gegl_rgb_float(self->pixel, 1.0, 1.0, 1.0);
}

static void
finalize(GObject *gobject)
{
  GeglColor *self = GEGL_COLOR (gobject);

  /* Delete the reference to the pixel*/
  g_free(self->pixel);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}


static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglColor *self = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_WIDTH:
      g_value_set_int (value, gegl_color_get_width(self));
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, gegl_color_get_height(self));
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      gegl_color_get_pixel(self, value); 
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
  GeglColor *self = GEGL_COLOR(gobject);
  switch (prop_id)
  {
    case PROP_WIDTH:
      gegl_color_set_width(self,g_value_get_int (value));
      break;
    case PROP_HEIGHT:
      gegl_color_set_height(self, g_value_get_int (value));
      break;
    case PROP_PIXEL_RGB_FLOAT:
    case PROP_PIXEL_RGB_UINT8:
      gegl_color_set_pixel(self, (GValue*)value); 
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_color_get_width:
 * @self: a #GeglColor. 
 *
 * Gets the width of the color image.
 *
 * Returns: width of the color image.
 **/
gint 
gegl_color_get_width (GeglColor * self)
{
  return self->width;
}

/**
 * gegl_color_set_width:
 * @self: a #GeglColor. 
 * @width: a integer width. 
 *
 * Sets the width of the color image.
 *
 **/
void
gegl_color_set_width (GeglColor * self, 
                              gint width)
{
  self->width = width;
}

/**
 * gegl_color_get_height:
 * @self: a #GeglColor. 
 *
 * Gets the height of the color image.
 *
 * Returns: height of the color image.
 **/
gint 
gegl_color_get_height (GeglColor * self)
{
  return self->height;
}

/**
 * gegl_color_set_height:
 * @self: a #GeglColor. 
 * @height: a integer height. 
 *
 * Sets the height of the color image.
 *
 **/
void
gegl_color_set_height (GeglColor * self, 
                       gint height)
{
  self->height = height;
}

static void 
compute_have_rect(GeglFilter * filter, 
                  GeglRect *have_rect, 
                  GList * input_have_rects)
{ 
  GeglColor * self = GEGL_COLOR(filter);

  have_rect->x = 0;
  have_rect->y = 0;
  have_rect->w = self->width;
  have_rect->h = self->height;
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                           GList *input_color_models)
{
  GeglColor* self = GEGL_COLOR(filter);

  GeglColorModel *color_cm = g_value_pixel_get_color_model(self->pixel);
  gegl_image_set_derived_color_model(GEGL_IMAGE(filter), color_cm);
  return color_cm;
}

void
gegl_color_get_pixel (GeglColor * self,
                      GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (g_value_type_transformable(G_VALUE_TYPE(self->pixel), G_VALUE_TYPE(pixel)));

  g_value_transform(self->pixel, pixel); 
}

void
gegl_color_set_pixel (GeglColor * self, 
                      GValue *pixel)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR (self));

  g_value_unset(self->pixel);
  g_value_init(self->pixel, G_VALUE_TYPE(pixel));
  g_value_copy(pixel, self->pixel); 
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  color_u8, 
  color_float, 
  color_u16 
};

static GeglScanlineFunc
get_scanline_func(GeglNoInput * no_input,
                  GeglColorSpaceType space,
                  GeglDataSpaceType type)
{
  return scanline_funcs[type];
}

static void                                                            
color_float(GeglFilter * filter,              
            GeglImageDataIterator ** iters,        
            gint width)                       
{                                                                       
  GeglColor * self = GEGL_COLOR(filter);
  gfloat *pixel = (gfloat*)g_value_pixel_get_data(self->pixel);
  gfloat **d = (gfloat**)gegl_image_data_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

#if 0
    g_print("color_float called %f %f %f \n", color_values[0].f, color_values[1].f, color_values[2].f);
    g_print("color dest addresses are %p %p %p \n", d0, d1, d2);
#endif

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
            case 0:        
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}                                                                       

static void                                                            
color_u16 (GeglFilter * filter,              
          GeglImageDataIterator ** iters,        
          gint width)                       
{                                                                       
  GeglColor * self = GEGL_COLOR(filter);
  guint16 *pixel = (guint16*)g_value_pixel_get_data(self->pixel);
  guint16 **d = (guint16**)gegl_image_data_iterator_color_channels(iters[0]);
  guint16 *da = (guint16*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    guint16 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint16 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint16 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
            case 0:        
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}                                                                       

static void                                                            
color_u8 (GeglFilter * filter,              
         GeglImageDataIterator ** iters,        
         gint width)                       
{                                                                       
  GeglColor * self = GEGL_COLOR(filter);
  guint8 *pixel = (guint8*)g_value_pixel_get_data(self->pixel);
  guint8 **d = (guint8**)gegl_image_data_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_image_data_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_data_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    guint8 *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    guint8 *d1 = (d_color_chans > 1) ? d[1]: NULL;
    guint8 *d2 = (d_color_chans > 2) ? d[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = pixel[2];
            case 2: *d1++ = pixel[1];
            case 1: *d0++ = pixel[0];
            case 0:        
          }

        if(has_alpha)
          *da++ = pixel[alpha];
      }
  }

  g_free(d);
}                                                                       
