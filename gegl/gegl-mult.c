#include "gegl-mult.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-attributes.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMultClass * klass);
static void init (GeglMult * self, GeglMultClass * klass);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);

static void scanline_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static void scanline_gray_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_mult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMultClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;

  return;
}

static void 
init (GeglMult * self, 
      GeglMultClass * klass)
{
  g_object_set(self, "num_inputs", 2, NULL);
}

static void 
prepare (GeglFilter * filter, 
         GList * output_attributes,
         GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter); 

  GeglAttributes *dest_attributes = 
    (GeglAttributes*)g_list_nth_data(output_attributes, 0); 
  GeglTile *dest = (GeglTile *)g_value_get_object(dest_attributes->value);
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest);

  g_return_if_fail(dest_cm);

  /* Get correct scanline func for this color model */
  {
    GeglChannelDataType type = gegl_color_model_data_type(dest_cm);
    GeglColorSpace space = gegl_color_model_color_space(dest_cm);

    if(space == GEGL_COLOR_SPACE_RGB)
      {
        if(type == GEGL_FLOAT)
          point_op->scanline_processor->func = scanline_rgb_float;        
        else if(type == GEGL_U16)
          point_op->scanline_processor->func = scanline_rgb_u16;        
        else if(type == GEGL_U8)
          point_op->scanline_processor->func = scanline_rgb_u8;        
      }
    else if(space == GEGL_COLOR_SPACE_GRAY)
      {
        if(type == GEGL_FLOAT)
          point_op->scanline_processor->func = scanline_gray_float;        
        else if(type == GEGL_U16)
          point_op->scanline_processor->func = scanline_gray_u16;        
        else if(type == GEGL_U8)
          point_op->scanline_processor->func = scanline_gray_u8;        
      }
    else 
      {
        g_error("Color Space not supported for MULT");
      }
  }
}

static void 
scanline_rgb_float (GeglFilter * filter, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  gfloat *dest_data[4];
  gboolean dest_has_alpha;
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  gfloat *src1_data[4];
  gboolean src1_has_alpha;
  gfloat *src1_r, *src1_g, *src1_b, *src1_alpha=NULL;
  gfloat *src2_data[4];
  gboolean src2_has_alpha;
  gfloat *src2_r, *src2_g, *src2_b, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  if (src1_has_alpha)
    src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  if (src2_has_alpha)
    src2_alpha = src2_data[3];

    
  while (width--)
    {
 
      *dest_r = *src1_r * *src2_r;
      *dest_g = *src1_g * *src2_g;
      *dest_b = *src1_b * *src2_b;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_r++;
      src1_g++;
      src1_b++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_r++;
      src2_g++;
      src2_b++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}

static void 
scanline_rgb_u16 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  guint16 *dest_data[4];
  gboolean dest_has_alpha;
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  guint16 *src1_data[4];
  gboolean src1_has_alpha;
  guint16 *src1_r, *src1_g, *src1_b, *src1_alpha=NULL;
  guint16 *src2_data[4];
  gboolean src2_has_alpha;
  guint16 *src2_r, *src2_g, *src2_b, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  if (src1_has_alpha)
    src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  if (src2_has_alpha)
    src2_alpha = src2_data[3];

    
  while (width--)
    {
 
      *dest_r = (*src1_r * *src2_r) / 65535UL ;
      *dest_g = (*src1_g * *src2_g) / 65535UL ;
      *dest_b = (*src1_b * *src2_b) / 65535UL ;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_r++;
      src1_g++;
      src1_b++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_r++;
      src2_g++;
      src2_b++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}

static void 
scanline_rgb_u8 (GeglFilter * filter, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  guint8 *dest_data[4];
  gboolean dest_has_alpha;
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  guint8 *src1_data[4];
  gboolean src1_has_alpha;
  guint8 *src1_r, *src1_g, *src1_b, *src1_alpha=NULL;
  guint8 *src2_data[4];
  gboolean src2_has_alpha;
  guint8 *src2_r, *src2_g, *src2_b, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  if (src1_has_alpha)
    src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  if (src2_has_alpha)
    src2_alpha = src2_data[3];

    
  while (width--)
    {
 
      *dest_r = (*src1_r * *src2_r) / 255L   ;
      *dest_g = (*src1_g * *src2_g) / 255L   ;
      *dest_b = (*src1_b * *src2_b) / 255L   ;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_r++;
      src1_g++;
      src1_b++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_r++;
      src2_g++;
      src2_b++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}

static void 
scanline_gray_float (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  gfloat *dest_data[2];
  gboolean dest_has_alpha;
  gfloat *dest_gray, *dest_alpha=NULL;
  gfloat *src1_data[2];
  gboolean src1_has_alpha;
  gfloat *src1_gray, *src1_alpha=NULL;
  gfloat *src2_data[2];
  gboolean src2_has_alpha;
  gfloat *src2_gray, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src1_gray = src1_data[0];
  if (src1_has_alpha)
    src1_alpha = src1_data[1];

  src2_gray = src2_data[0];
  if (src2_has_alpha)
    src2_alpha = src2_data[1];

    
  while (width--)
    {
 
      *dest_gray = *src1_gray * *src2_gray;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_gray++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_gray++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}

static void 
scanline_gray_u16 (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  guint16 *dest_data[2];
  gboolean dest_has_alpha;
  guint16 *dest_gray, *dest_alpha=NULL;
  guint16 *src1_data[2];
  gboolean src1_has_alpha;
  guint16 *src1_gray, *src1_alpha=NULL;
  guint16 *src2_data[2];
  gboolean src2_has_alpha;
  guint16 *src2_gray, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src1_gray = src1_data[0];
  if (src1_has_alpha)
    src1_alpha = src1_data[1];

  src2_gray = src2_data[0];
  if (src2_has_alpha)
    src2_alpha = src2_data[1];

    
  while (width--)
    {
 
      *dest_gray = (*src1_gray * *src2_gray) / 65535UL ;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_gray++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_gray++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}

static void 
scanline_gray_u8 (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src1_cm = gegl_tile_iterator_get_color_model(iters[1]);
  GeglColorModel *src2_cm = gegl_tile_iterator_get_color_model(iters[2]);

  guint8 *dest_data[2];
  gboolean dest_has_alpha;
  guint8 *dest_gray, *dest_alpha=NULL;
  guint8 *src1_data[2];
  gboolean src1_has_alpha;
  guint8 *src1_gray, *src1_alpha=NULL;
  guint8 *src2_data[2];
  gboolean src2_has_alpha;
  guint8 *src2_gray, *src2_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src1_has_alpha = gegl_color_model_has_alpha(src1_cm); 
  src2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src1_gray = src1_data[0];
  if (src1_has_alpha)
    src1_alpha = src1_data[1];

  src2_gray = src2_data[0];
  if (src2_has_alpha)
    src2_alpha = src2_data[1];

    
  while (width--)
    {
 
      *dest_gray = (*src1_gray * *src2_gray) / 255L   ;
 
      if (dest_has_alpha)
        {
          if (src1_has_alpha && src2_has_alpha)
            {
          *dest_alpha = MIN (*src1_alpha,*src2_alpha);
            }
          else if (src1_has_alpha)
            {
          *dest_alpha = *src1_alpha;
            }
          else if (src2_has_alpha)
            {
          *dest_alpha = *src2_alpha;
            }
        }
 
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src1_gray++;
      if (src1_has_alpha)
        src1_alpha++;

      src2_gray++;
      if (src2_has_alpha)
        src2_alpha++;

    }               
}
