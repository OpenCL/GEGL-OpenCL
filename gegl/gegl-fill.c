#include "gegl-fill.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

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
static void compute_have_rect(GeglFilter * filter, GeglRect *have_rect, GList * input_have_rects);

static void prepare (GeglFilter * filter, GList * attributes, GList * input_attributes);

static void scanline_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

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

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
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

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize= finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->prepare = prepare;
  filter_class->compute_derived_color_model = compute_derived_color_model;
  filter_class->compute_have_rect = compute_have_rect;

  g_object_class_install_property (gobject_class, PROP_FILLCOLOR,
                                   g_param_spec_object ("fillcolor",
                                                        "FillColor",
                                                        "The GeglFill's fill color",
                                                         GEGL_TYPE_COLOR,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));
  return;
}

static void 
init (GeglFill * self, 
      GeglFillClass * klass)
{
  GeglNode * node = GEGL_NODE(self); 
  gegl_node_set_num_outputs(node,1);
  gegl_node_set_num_inputs(node,0);
  return;
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
  GeglFill *self = GEGL_FILL (gobject);

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
  GeglFill *self = GEGL_FILL (gobject);

  switch (prop_id)
  {
    case PROP_FILLCOLOR:
        gegl_fill_set_fill_color (self,(GeglColor*)(g_value_get_object(value)));
      break;
    default:
      break;
  }
}

static void 
compute_have_rect(GeglFilter * op, 
                  GeglRect *have_rect, 
                  GList * input_have_rects)
{ 
  gegl_rect_set(have_rect, 0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT);
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
   
  self->fill_color = fill_color;

  if(fill_color)
    g_object_ref(G_OBJECT(fill_color));
}

static void 
prepare (GeglFilter * filter, 
         GList * attributes,
         GList * input_attributes)
{
  GeglFill * self = GEGL_FILL(filter); 
  GeglPointOp *point_op = GEGL_POINT_OP(filter); 
  GeglAttributes *dest_attributes = 
    (GeglAttributes*)g_list_nth_data(attributes, 0); 
  GeglTile *dest = (GeglTile*)g_value_get_object(dest_attributes->value);
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest);
  GeglColorModel * fill_cm = gegl_color_get_color_model (self->fill_color);

  g_return_if_fail (dest_cm == fill_cm);

  {
    GeglColorSpace colorspace = gegl_color_model_color_space(fill_cm);
    GeglChannelDataType data_type = gegl_color_model_data_type (fill_cm);

    switch (colorspace)
      {
        case GEGL_COLOR_SPACE_RGB:
          switch (data_type)
            {
            case GEGL_FLOAT:
              point_op->scanline_processor->func = scanline_rgb_float;
              break;
            case GEGL_U8:
              point_op->scanline_processor->func = scanline_rgb_u8;
              break;
            case GEGL_U16:
              point_op->scanline_processor->func = scanline_rgb_u16;
              break;
            default:
              g_warning("prepare: Can't find data_type\n");    
              break;

            }
          break;
        case GEGL_COLOR_SPACE_GRAY:
          switch (data_type)
            {
            case GEGL_FLOAT:
              point_op->scanline_processor->func = scanline_gray_float;
              break;
            case GEGL_U8:
              point_op->scanline_processor->func = scanline_gray_u8;
              break;
            case GEGL_U16:
              point_op->scanline_processor->func = scanline_gray_u16;
              break;
            default:
              g_warning("prepare: Can't find data_type\n");    
              break;
            }
          break;
        default:
          g_warning("prepare: Can't find colorspace\n");    
          break;
      }
  }
}

/**
 * scanline_rgb_u8:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u8 (GeglFilter * filter, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint8 *dest_data[4];
  gboolean dest_has_alpha;
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  
  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  /* Fill the dest with the fill color */
  while (width--)
    { 
      *dest_r = fill_values[0].u8;
      *dest_g = fill_values[1].u8;
      *dest_b = fill_values[2].u8;
      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[3].u8;
        }
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}

/**
 * scanline_rgb_float:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_float (GeglFilter * filter, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  gfloat *dest_data[4];
  gboolean dest_has_alpha;
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  
  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  /* Fill the dest with the fill color */

  while (width--)
    { 
      *dest_r = fill_values[0].f;
      *dest_g = fill_values[1].f;
      *dest_b = fill_values[2].f;

      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[3].f;
        }
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}

/**
 * scanline_rgb_u16:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u16 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint16 *dest_data[4];
  gboolean dest_has_alpha;
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  /* Fill the dest with the fill color */
  while (width--)
    { 
      *dest_r = fill_values[0].u16;
      *dest_g = fill_values[1].u16;
      *dest_b = fill_values[2].u16;
      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[3].u16;
        }
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}

/**
 * scanline_gray_u8:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u8 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint8 *dest_data[2];
  gboolean dest_has_alpha;
  guint8 *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  /* Fill the dest with the fill color */
  while (width--)
    { 
      *dest_gray = fill_values[0].u8;
      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[1].u8;
        }
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}

/**
 * scanline_gray_float:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_float (GeglFilter * filter, 
                     GeglTileIterator ** iters, 
                     gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  gfloat *dest_data[2];
  gboolean dest_has_alpha;
  gfloat *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  /* Fill the dest with the fill color */
  while (width--)
    { 
      *dest_gray = fill_values[0].f;
      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[1].f;
        }
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}

/**
 * scanline_gray_u16:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u16 (GeglFilter * filter, 
                   GeglTileIterator ** iters, 
                   gint width)
{
  GeglFill *self = GEGL_FILL(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglChannelValue *fill_values = gegl_color_get_channel_values (self->fill_color);

  guint16 *dest_data[2];
  gboolean dest_has_alpha;
  guint16 *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  /* Fill the dest with the fill color */
  while (width--)
    { 
      *dest_gray = fill_values[0].u16;
      if (dest_has_alpha)
        { 
          *dest_alpha = fill_values[1].u16;
        }
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;
    }
}
