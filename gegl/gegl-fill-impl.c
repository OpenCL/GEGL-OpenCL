#include "gegl-fill-impl.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"

static void class_init (GeglFillImplClass * klass);
static void init (GeglFillImpl * self, GeglFillImplClass * klass);
static void finalize (GObject * gobject);

static void prepare (GeglOpImpl * self_op_impl, GList * requests);
static void scanline_rgb_u8 (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);
static void scanline_rgb_float (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);
static void scanline_gray_float (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_fill_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFillImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFillImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP_IMPL, 
                                     "GeglFillImpl", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFillImplClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpImplClass *op_impl_class = GEGL_OP_IMPL_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize= finalize;

  op_impl_class->prepare = prepare;

  return;
}

static void 
init (GeglFillImpl * self, 
      GeglFillImplClass * klass)
{
  GeglOpImpl * self_op_impl = GEGL_OP_IMPL(self);
  self_op_impl->num_inputs = 0;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglFillImpl *self = GEGL_FILL_IMPL (gobject);

  /* Delete the reference to the color*/
  if(self->fill_color)
    g_object_unref(G_OBJECT(self->fill_color));

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

GeglColor * 
gegl_fill_impl_get_fill_color (GeglFillImpl * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILL_IMPL (self), NULL);
   
  return self->fill_color;
}

void
gegl_fill_impl_set_fill_color (GeglFillImpl * self, 
                             GeglColor *fill_color)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILL_IMPL (self));
   
  self->fill_color = fill_color;
  if(fill_color)
    g_object_ref(G_OBJECT(fill_color));
}

static void 
prepare (GeglOpImpl * self_op_impl, 
         GList * requests)
{
  GeglFillImpl * self = GEGL_FILL_IMPL(self_op_impl); 
  GeglPointOpImpl *self_point_op_impl = GEGL_POINT_OP_IMPL(self_op_impl); 
  GeglOpRequest *dest_request = (GeglOpRequest*)g_list_nth_data(requests,0); 
  GeglTile *dest = dest_request->tile;
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
              self_point_op_impl->scanline_processor->func = scanline_rgb_float;
              break;
            case GEGL_U8:
              self_point_op_impl->scanline_processor->func = scanline_rgb_u8;
              break;
            case GEGL_U16:
              self_point_op_impl->scanline_processor->func = scanline_rgb_u16;
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
              self_point_op_impl->scanline_processor->func = scanline_gray_float;
              break;
            case GEGL_U8:
              self_point_op_impl->scanline_processor->func = scanline_gray_u8;
              break;
            case GEGL_U16:
              self_point_op_impl->scanline_processor->func = scanline_gray_u16;
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u8 (GeglOpImpl * self_op_impl, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_float (GeglOpImpl * self_op_impl, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u16 (GeglOpImpl * self_op_impl, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u8 (GeglOpImpl * self_op_impl, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_float (GeglOpImpl * self_op_impl, 
                     GeglTileIterator ** iters, 
                     gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
 * @self_op_impl: a #GeglOpImpl.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u16 (GeglOpImpl * self_op_impl, 
                   GeglTileIterator ** iters, 
                   gint width)
{
  GeglFillImpl *self = GEGL_FILL_IMPL(self_op_impl);
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
