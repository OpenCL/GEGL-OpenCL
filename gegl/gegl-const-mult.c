#include "gegl-const-mult.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-attributes.h"
#include "gegl-value-types.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_MULTIPLIER,
  PROP_LAST 
};

static void class_init (GeglConstMultClass * klass);
static void init (GeglConstMult * self, GeglConstMultClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);

static void scanline_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static void scanline_gray_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

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

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglConstMult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
init (GeglConstMult * self, 
      GeglConstMultClass * klass)
{
  g_object_set(self, "num_inputs", 1, NULL);
  return;
}

static void 
class_init (GeglConstMultClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
   
  g_object_class_install_property (gobject_class,
                                   PROP_MULTIPLIER,
                                   g_param_spec_float ("multiplier",
                                                       "GeglConstMult Multiplier",
                                                       "Multiply source pixels by constant multiplier",
                                                       G_MINFLOAT, 
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));

  return;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglConstMult *self = GEGL_CONST_MULT (gobject);

  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      g_value_set_float (value, gegl_const_mult_get_multiplier(self));
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
  GeglConstMult *self = GEGL_CONST_MULT (gobject);

  switch (prop_id)
  {
    case PROP_MULTIPLIER:
      gegl_const_mult_set_multiplier (self,g_value_get_float(value));
      break;
    default:
      break;
  }
}

gfloat
gegl_const_mult_get_multiplier (GeglConstMult * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_CONST_MULT (self), 0);
   
  return self->multiplier;
}

void
gegl_const_mult_set_multiplier(GeglConstMult * self, 
                               gfloat multiplier)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_CONST_MULT (self));
   
  self->multiplier = multiplier;
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
        g_error("Color Space not supported for ADD");
      }
  }
}

static void 
scanline_rgb_float (GeglFilter * filter, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  gfloat *dest_data[4];
  gboolean dest_has_alpha;
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  gfloat *src_data[4];
  gboolean src_has_alpha;
  gfloat *src_r, *src_g, *src_b, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];


  while (width--)
    {
      *dest_r = multiplier * *src_r;
      *dest_g = multiplier * *src_g;
      *dest_b = multiplier * *src_b;
      if (dest_has_alpha)
        *dest_alpha = multiplier * *src_alpha;
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src_r++;
      src_g++;
      src_b++;
      if (src_has_alpha)
        src_alpha++;

    }
}

static void 
scanline_rgb_u16 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  guint16 *dest_data[4];
  gboolean dest_has_alpha;
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  guint16 *src_data[4];
  gboolean src_has_alpha;
  guint16 *src_r, *src_g, *src_b, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];


  while (width--)
    {
      *dest_r = ROUND (multiplier * *src_r);
      *dest_g = ROUND (multiplier * *src_g);
      *dest_b = ROUND (multiplier * *src_b);
      if (dest_has_alpha)
        *dest_alpha = ROUND (multiplier * *src_alpha);
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src_r++;
      src_g++;
      src_b++;
      if (src_has_alpha)
        src_alpha++;

    }
}

static void 
scanline_rgb_u8 (GeglFilter * filter, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  guint8 *dest_data[4];
  gboolean dest_has_alpha;
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;
  guint8 *src_data[4];
  gboolean src_has_alpha;
  guint8 *src_r, *src_g, *src_b, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];


  while (width--)
    {
      *dest_r = ROUND (multiplier * *src_r);
      *dest_g = ROUND (multiplier * *src_g);
      *dest_b = ROUND (multiplier * *src_b);
      if (dest_has_alpha)
        *dest_alpha = ROUND (multiplier * *src_alpha);
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      src_r++;
      src_g++;
      src_b++;
      if (src_has_alpha)
        src_alpha++;

    }
}

static void 
scanline_gray_float (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  gfloat *dest_data[2];
  gboolean dest_has_alpha;
  gfloat *dest_gray, *dest_alpha=NULL;
  gfloat *src_data[2];
  gboolean src_has_alpha;
  gfloat *src_gray, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];


  while (width--)
    {
      *dest_gray = multiplier * *src_gray;
      if (dest_has_alpha)
        *dest_alpha = multiplier * *src_alpha;
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src_gray++;
      if (src_has_alpha)
        src_alpha++;

    }
}

static void 
scanline_gray_u16 (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  guint16 *dest_data[2];
  gboolean dest_has_alpha;
  guint16 *dest_gray, *dest_alpha=NULL;
  guint16 *src_data[2];
  gboolean src_has_alpha;
  guint16 *src_gray, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];


  while (width--)
    {
      *dest_gray = ROUND (multiplier * *src_gray);
      if (dest_has_alpha)
        *dest_alpha = ROUND (multiplier * *src_alpha);
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src_gray++;
      if (src_has_alpha)
        src_alpha++;

    }
}

static void 
scanline_gray_u8 (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglConstMult *self = GEGL_CONST_MULT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  guint8 *dest_data[2];
  gboolean dest_has_alpha;
  guint8 *dest_gray, *dest_alpha=NULL;
  guint8 *src_data[2];
  gboolean src_has_alpha;
  guint8 *src_gray, *src_alpha=NULL;
  float multiplier;

  multiplier = self->multiplier;
  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 
  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];


  while (width--)
    {
      *dest_gray = ROUND (multiplier * *src_gray);
      if (dest_has_alpha)
        *dest_alpha = ROUND (multiplier * *src_alpha);
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      src_gray++;
      if (src_has_alpha)
        src_alpha++;

    }
}
