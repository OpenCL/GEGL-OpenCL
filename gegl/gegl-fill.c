#include "gegl-fill.h"
#include "gegl-scanline-processor.h"
#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_FILLCOLOR,
  PROP_LAST 
};

static void class_init (GeglFillClass * klass);
static void init (GeglFill * self, GeglFillClass * klass);
static void finalize (GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static GeglColorModel * compute_derived_color_model (GeglFilter * filter, GList * input_color_models);

static GeglScanlineFunc get_scanline_func(GeglNoInput * no_input, GeglColorSpace space, GeglChannelDataType type);

static void fill_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void fill_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void fill_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_fill_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFillClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFill),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NO_INPUT, 
                                     "GeglFill", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFillClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglNoInputClass *no_input_class = GEGL_NO_INPUT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  no_input_class->get_scanline_func = get_scanline_func;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  filter_class->compute_derived_color_model = compute_derived_color_model;

  g_object_class_install_property (gobject_class, PROP_FILLCOLOR,
                                   g_param_spec_object ("fillcolor",
                                                        "FillColor",
                                                        "The GeglFill's fill color",
                                                         GEGL_TYPE_COLOR,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglFill * self, 
      GeglFillClass * klass)
{
  self->fill_color = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglFill *self = GEGL_FILL (gobject);

  /* Delete the reference to the color*/
  if(self->fill_color)
    g_object_unref(self->fill_color);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglFill *self = GEGL_FILL(gobject);
  switch (prop_id)
  {
    case PROP_FILLCOLOR:
        g_value_set_object (value, (GeglColor*)(gegl_fill_get_fill_color(self)));
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
  GeglFill *self = GEGL_FILL(gobject);
  switch (prop_id)
  {
    case PROP_FILLCOLOR:
        gegl_fill_set_fill_color (self,(GeglColor*)(g_value_get_object(value)));
      break;
    default:
      break;
  }
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                             GList *input_color_models)
{
  GeglFill* self = GEGL_FILL(filter);

  GeglColorModel *fill_cm = gegl_color_get_color_model(self->fill_color);
  gegl_image_set_derived_color_model(GEGL_IMAGE(filter), fill_cm);
  return fill_cm;
}

GeglColor * 
gegl_fill_get_fill_color (GeglFill * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILL (self), NULL);
   
  return self->fill_color;
}

void
gegl_fill_set_fill_color (GeglFill * self, 
                          GeglColor *fill_color)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILL (self));

  if(fill_color)
    g_object_ref(fill_color);

  if(self->fill_color)
    g_object_unref(self->fill_color);
   
  self->fill_color = fill_color;
}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  fill_u8, 
  fill_float, 
  fill_u16 
};

static GeglScanlineFunc
get_scanline_func(GeglNoInput * no_input,
                  GeglColorSpace space,
                  GeglChannelDataType type)
{
  return scanline_funcs[type];
}

static void                                                            
fill_float (GeglFilter * filter,              
            GeglTileIterator ** iters,        
            gint width)                       
{                                                                       
  GeglFill * self = GEGL_FILL(filter);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  gfloat **d = (gfloat**)gegl_tile_iterator_color_channels(iters[0]);
  gfloat *da = (gfloat*)gegl_tile_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_tile_iterator_get_num_colors(iters[0]);
  gboolean has_alpha = da ? TRUE: FALSE;
  gint alpha = d_color_chans;   /* should get from color model */

  {
    gfloat *d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gfloat *d1 = (d_color_chans > 1) ? d[1]: NULL;
    gfloat *d2 = (d_color_chans > 2) ? d[2]: NULL;

    while(width--)                                                        
      {                                                                   
        switch(d_color_chans)
          {
            case 3: *d2++ = fill_values[2].f;
            case 2: *d1++ = fill_values[1].f;
            case 1: *d0++ = fill_values[0].f;
            case 0:        
          }

        if(has_alpha)
          *da++ = fill_values[alpha].f;
      }
  }

  g_free(d);
}                                                                       

static void                                                            
fill_u16 (GeglFilter * filter,              
          GeglTileIterator ** iters,        
          gint width)                       
{                                                                       
  GeglFill * self = GEGL_FILL(filter);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint16 **d = (guint16**)gegl_tile_iterator_color_channels(iters[0]);
  guint16 *da = (guint16*)gegl_tile_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_tile_iterator_get_num_colors(iters[0]);
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
            case 3: *d2++ = fill_values[2].u16;
            case 2: *d1++ = fill_values[1].u16;
            case 1: *d0++ = fill_values[0].u16;
            case 0:        
          }

        if(has_alpha)
          *da++ = fill_values[alpha].u16;
      }
  }

  g_free(d);
}                                                                       

static void                                                            
fill_u8 (GeglFilter * filter,              
         GeglTileIterator ** iters,        
         gint width)                       
{                                                                       
  GeglFill * self = GEGL_FILL(filter);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint8 **d = (guint8**)gegl_tile_iterator_color_channels(iters[0]);
  guint8 *da = (guint8*)gegl_tile_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_tile_iterator_get_num_colors(iters[0]);
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
            case 3: *d2++ = fill_values[2].u8;
            case 2: *d1++ = fill_values[1].u8;
            case 1: *d0++ = fill_values[0].u8;
            case 0:        
          }

        if(has_alpha)
          *da++ = fill_values[alpha].u8;
      }
  }

  g_free(d);
}                                                                       
