#include "gegl-comp-premult.h"
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
  PROP_COMPOSITE_MODE,
  PROP_LAST 
};

static void class_init (GeglCompPremultClass * klass);
static void init (GeglCompPremult * self, GeglCompPremultClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);

static void scanline_ca_op_ca_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_ca_op_c_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_ca_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_c_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static void scanline_ca_op_ca_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_ca_op_c_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_ca_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_c_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static void scanline_ca_op_ca_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_ca_op_c_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_ca_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_c_op_c_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_comp_premult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCompPremultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCompPremult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglCompPremult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCompPremultClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->prepare = prepare;

  g_object_class_install_property (gobject_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_int ("composite_mode",
                                                      "CompositeMode",
                                                      "The composite mode.",
                                                      GEGL_COMPOSITE_MODE_NONE,
                                                      GEGL_COMPOSITE_MODE_XOR,
                                                      GEGL_COMPOSITE_MODE_OVER,
                                                      G_PARAM_READWRITE));
}

static void 
init (GeglCompPremult * self, 
      GeglCompPremultClass * klass)
{
  self->composite_mode = GEGL_COMPOSITE_MODE_OVER;
  g_object_set(self, "num_inputs", 2, NULL);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglCompPremult *comp_premult = GEGL_COMP_PREMULT (gobject);
  switch (prop_id)
  {
    case PROP_COMPOSITE_MODE:
      g_value_set_int(value, gegl_comp_premult_get_composite_mode(comp_premult));  
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
  GeglCompPremult *comp_premult = GEGL_COMP_PREMULT (gobject);
  switch (prop_id)
  {
    case PROP_COMPOSITE_MODE:
      gegl_comp_premult_set_composite_mode(comp_premult, (GeglCompositeMode)g_value_get_int(value));  
      break;
    default:
      break;
  }
}

/**
 * gegl_comp_premult_get_composite_mode:
 * @self: a #GeglCompPremult.
 *
 * Gets the composite mode.
 *
 * Returns: composite mode. 
 **/
GeglCompositeMode
gegl_comp_premult_get_composite_mode (GeglCompPremult * self)
{
  g_return_val_if_fail (self != NULL, GEGL_COMPOSITE_MODE_NONE);
  g_return_val_if_fail (GEGL_IS_COMP_PREMULT (self), GEGL_COMPOSITE_MODE_NONE);

  return self->composite_mode;
}


/**
 * gegl_comp_premult_set_composite_mode:
 * @self: a #GeglCompPremult.
 * @mode: the composite mode. 
 *
 * Sets the composite mode. 
 *
 **/
void 
gegl_comp_premult_set_composite_mode (GeglCompPremult * self, 
                                      GeglCompositeMode mode)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COMP_PREMULT (self));

  self->composite_mode = mode;
}

static void 
prepare (GeglFilter * filter, 
         GList * output_attributes,
         GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);

  GeglAttributes *dest_attr = g_list_nth_data(output_attributes, 0);
  GeglTile *dest = (GeglTile*)g_value_get_object(dest_attr->value);
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest);

  GeglAttributes *src1_attr = g_list_nth_data(input_attributes, 0);
  GeglTile *src1 = (GeglTile*)g_value_get_object(src1_attr->value);
  GeglColorModel * src1_cm = gegl_tile_get_color_model (src1);

  GeglAttributes *src2_attr = g_list_nth_data(input_attributes, 1);
  GeglTile *src2 = (GeglTile*)g_value_get_object(src2_attr->value);
  GeglColorModel * src2_cm = gegl_tile_get_color_model (src2);

  g_return_if_fail (dest_cm);
  g_return_if_fail (src1_cm);
  g_return_if_fail (src2_cm);

  {
    GeglChannelDataType type = gegl_color_model_data_type(dest_cm);
    GeglColorSpace space = gegl_color_model_color_space(dest_cm);
    gboolean  s1_has_alpha = gegl_color_model_has_alpha(src1_cm);
    gboolean  s2_has_alpha = gegl_color_model_has_alpha(src2_cm); 

    /* dest = src2 op src1 */ 
    if(space == GEGL_COLOR_SPACE_RGB)
      {
        if(type == GEGL_FLOAT)
          {
            if ( s2_has_alpha && s1_has_alpha)
              point_op->scanline_processor->func = scanline_ca_op_ca_rgb_float;
            else if ( s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_ca_op_c_rgb_float;
            else if ( !s2_has_alpha && s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_ca_rgb_float;
            else if ( !s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_c_rgb_float;
          }
        else if(type == GEGL_U16)
          {
            if ( s2_has_alpha && s1_has_alpha)
              point_op->scanline_processor->func = scanline_ca_op_ca_rgb_u16;
            else if ( s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_ca_op_c_rgb_u16;
            else if ( !s2_has_alpha && s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_ca_rgb_u16;
            else if ( !s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_c_rgb_u16;
          } 
        else if(type == GEGL_U8)
          {
            if ( s2_has_alpha && s1_has_alpha)
              point_op->scanline_processor->func = scanline_ca_op_ca_rgb_u8;
            else if ( s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_ca_op_c_rgb_u8;
            else if ( !s2_has_alpha && s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_ca_rgb_u8;
            else if ( !s2_has_alpha && !s1_has_alpha )
              point_op->scanline_processor->func = scanline_c_op_c_rgb_u8;
          } 
      }

  }
}


static void 
scanline_ca_op_ca_rgb_float (GeglFilter * filter, 
                             GeglTileIterator ** iters, 
                             gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  gfloat *dest_data[4];
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha;
  gfloat *src1_data[4];
  gfloat *src1_r, *src1_g, *src1_b, *src1_alpha;
  gfloat *src2_data[4];
  gfloat *src2_r, *src2_g, *src2_b, *src2_alpha;
  gfloat a, b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--) 
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          *dest_r = a * *src1_r + *src2_r;
          *dest_g = a * *src1_g + *src2_g;
          *dest_b = a * *src1_b + *src2_b;
          *dest_alpha = a * *src1_alpha + *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        } 
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b * *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 1.0 - *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b * *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          b = *src1_alpha;
          *dest_r = a * *src1_r + b * *src2_r;
          *dest_g = a * *src1_g + b * *src2_g;
          *dest_b = a * *src1_b + b * *src2_b;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          b = 1.0 - *src1_alpha;
          *dest_r = a * *src1_r + b * *src2_r;
          *dest_g = a * *src1_g + b * *src2_g;
          *dest_b = a * *src1_b + b * *src2_b;
          *dest_alpha = a * *src1_alpha + b * *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    }  
}

static void 
scanline_ca_op_c_rgb_float (GeglFilter * filter, 
                            GeglTileIterator ** iters, 
                            gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  gfloat *dest_data[3];
  gfloat *dest_r, *dest_g, *dest_b;
  gfloat *src1_data[3];
  gfloat *src1_r, *src1_g, *src1_b;
  gfloat *src2_data[4];
  gfloat *src2_r, *src2_g, *src2_b, *src2_alpha;
  gfloat a;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          *dest_r = a * *src1_r + *src2_r;
          *dest_g = a * *src1_g + *src2_g;
          *dest_b = a * *src1_b + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          *dest_r = 0.0;
          *dest_g = 0.0;
          *dest_b = 0.0;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          *dest_r = a * *src1_r + *src2_r;
          *dest_g = a * *src1_g + *src2_g;
          *dest_b = a * *src1_b + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 1.0 - *src2_alpha;
          *dest_r = a * *src1_r;
          *dest_g = a * *src1_g;
          *dest_b = a * *src1_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    } 
}

static void 
scanline_c_op_ca_rgb_float (GeglFilter * filter, 
                            GeglTileIterator ** iters, 
                            gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  gfloat *dest_data[4];
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha;
  gfloat *src1_data[4];
  gfloat *src1_r, *src1_g, *src1_b, *src1_alpha;
  gfloat *src2_data[3];
  gfloat *src2_r, *src2_g, *src2_b;
  gfloat b; 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 1.0;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 1.0;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 1.0 - *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          b = 1.0 - *src1_alpha;
          *dest_r = b * *src2_r;
          *dest_g = b * *src2_g;
          *dest_b = b * *src2_b;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    default:
      break;
    } 

}

static void 
scanline_c_op_c_rgb_float (GeglFilter * filter, 
                           GeglTileIterator ** iters, 
                           gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  gfloat *dest_data[3];
  gfloat *dest_r, *dest_g, *dest_b;
  gfloat *src1_data[3];
  gfloat *src1_r, *src1_g, *src1_b;
  gfloat *src2_data[3];
  gfloat *src2_r, *src2_g, *src2_b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
    case GEGL_COMPOSITE_MODE_OVER:
    case GEGL_COMPOSITE_MODE_IN:
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
          *dest_r = 0.0;
          *dest_g = 0.0;
          *dest_b = 0.0;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    default:
      break;
    }

}

static void 
scanline_ca_op_ca_rgb_u16 (GeglFilter * filter, 
                   GeglTileIterator ** iters, 
                   gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint16 *dest_data[4];
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha;
  guint16 *src1_data[4];
  guint16 *src1_r, *src1_g, *src1_b, *src1_alpha;
  guint16 *src2_data[4];
  guint16 *src2_r, *src2_g, *src2_b, *src2_alpha;
  guint16 a, b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--) 
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          *dest_r = (a * *src1_r) / 65535UL  + *src2_r;
          *dest_g = (a * *src1_g) / 65535UL  + *src2_g;
          *dest_b = (a * *src1_b) / 65535UL  + *src2_b;
          *dest_alpha = (a * *src1_alpha) / 65535UL  + *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        } 
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = (b * *src2_alpha) / 65535UL ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 65535UL - *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = (b * *src2_alpha) / 65535UL ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          b = *src1_alpha;
          *dest_r = (a * *src1_r) / 65535UL  + (b * *src2_r) / 65535UL ;
          *dest_g = (a * *src1_g) / 65535UL  + (b * *src2_g) / 65535UL ;
          *dest_b = (a * *src1_b) / 65535UL  + (b * *src2_b) / 65535UL ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          b = 65535UL - *src1_alpha;
          *dest_r = (a * *src1_r) / 65535UL  + (b * *src2_r) / 65535UL ;
          *dest_g = (a * *src1_g) / 65535UL  + (b * *src2_g) / 65535UL ;
          *dest_b = (a * *src1_b) / 65535UL  + (b * *src2_b) / 65535UL ;
          *dest_alpha = (a * *src1_alpha) / 65535UL  + (b * *src2_alpha) / 65535UL ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    }  
}

static void 
scanline_ca_op_c_rgb_u16 (GeglFilter * filter, 
                          GeglTileIterator ** iters, 
                          gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint16 *dest_data[3];
  guint16 *dest_r, *dest_g, *dest_b;
  guint16 *src1_data[3];
  guint16 *src1_r, *src1_g, *src1_b;
  guint16 *src2_data[4];
  guint16 *src2_r, *src2_g, *src2_b, *src2_alpha;
  guint16 a;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          *dest_r = (a * *src1_r) / 65535UL  + *src2_r;
          *dest_g = (a * *src1_g) / 65535UL  + *src2_g;
          *dest_b = (a * *src1_b) / 65535UL  + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          *dest_r = 0UL;
          *dest_g = 0UL;
          *dest_b = 0UL;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          *dest_r = (a * *src1_r) / 65535UL  + *src2_r;
          *dest_g = (a * *src1_g) / 65535UL  + *src2_g;
          *dest_b = (a * *src1_b) / 65535UL  + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 65535UL - *src2_alpha;
          *dest_r = (a * *src1_r) / 65535UL ;
          *dest_g = (a * *src1_g) / 65535UL ;
          *dest_b = (a * *src1_b) / 65535UL ;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    } 
}

static void 
scanline_c_op_ca_rgb_u16 (GeglFilter * filter, 
                          GeglTileIterator ** iters, 
                          gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint16 *dest_data[4];
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha;
  guint16 *src1_data[4];
  guint16 *src1_r, *src1_g, *src1_b, *src1_alpha;
  guint16 *src2_data[3];
  guint16 *src2_r, *src2_g, *src2_b;
  guint16 b; 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 65535UL;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 65535UL;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 65535UL - *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          b = 65535UL - *src1_alpha;
          *dest_r = (b * *src2_r) / 65535UL ;
          *dest_g = (b * *src2_g) / 65535UL ;
          *dest_b = (b * *src2_b) / 65535UL ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    default:
      break;
    } 

}

static void 
scanline_c_op_c_rgb_u16 (GeglFilter * filter, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint16 *dest_data[3];
  guint16 *dest_r, *dest_g, *dest_b;
  guint16 *src1_data[3];
  guint16 *src1_r, *src1_g, *src1_b;
  guint16 *src2_data[3];
  guint16 *src2_r, *src2_g, *src2_b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
    case GEGL_COMPOSITE_MODE_OVER:
    case GEGL_COMPOSITE_MODE_IN:
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
          *dest_r = 0UL;
          *dest_g = 0UL;
          *dest_b = 0UL;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    default:
      break;
    }

}

static void 
scanline_ca_op_ca_rgb_u8 (GeglFilter * filter, 
                          GeglTileIterator ** iters, 
                          gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint8 *dest_data[4];
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha;
  guint8 *src1_data[4];
  guint8 *src1_r, *src1_g, *src1_b, *src1_alpha;
  guint8 *src2_data[4];
  guint8 *src2_r, *src2_g, *src2_b, *src2_alpha;
  guint8 a, b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--) 
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          *dest_r = (a * *src1_r) / 255L    + *src2_r;
          *dest_g = (a * *src1_g) / 255L    + *src2_g;
          *dest_b = (a * *src1_b) / 255L    + *src2_b;
          *dest_alpha = (a * *src1_alpha) / 255L    + *src2_alpha;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        } 
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = (b * *src2_alpha) / 255L   ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 255L - *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = (b * *src2_alpha) / 255L   ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          b = *src1_alpha;
          *dest_r = (a * *src1_r) / 255L    + (b * *src2_r) / 255L   ;
          *dest_g = (a * *src1_g) / 255L    + (b * *src2_g) / 255L   ;
          *dest_b = (a * *src1_b) / 255L    + (b * *src2_b) / 255L   ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          b = 255L - *src1_alpha;
          *dest_r = (a * *src1_r) / 255L    + (b * *src2_r) / 255L   ;
          *dest_g = (a * *src1_g) / 255L    + (b * *src2_g) / 255L   ;
          *dest_b = (a * *src1_b) / 255L    + (b * *src2_b) / 255L   ;
          *dest_alpha = (a * *src1_alpha) / 255L    + (b * *src2_alpha) / 255L   ;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    }  
}

static void 
scanline_ca_op_c_rgb_u8 (GeglFilter * filter, 
                         GeglTileIterator ** iters, 
                         gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint8 *dest_data[3];
  guint8 *dest_r, *dest_g, *dest_b;
  guint8 *src1_data[3];
  guint8 *src1_r, *src1_g, *src1_b;
  guint8 *src2_data[4];
  guint8 *src2_r, *src2_g, *src2_b, *src2_alpha;
  guint8 a;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];
  src2_alpha = src2_data[3];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          *dest_r = (a * *src1_r) / 255L    + *src2_r;
          *dest_g = (a * *src1_g) / 255L    + *src2_g;
          *dest_b = (a * *src1_b) / 255L    + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          *dest_r = 0	;
          *dest_g = 0	;
          *dest_b = 0	;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          *dest_r = (a * *src1_r) / 255L    + *src2_r;
          *dest_g = (a * *src1_g) / 255L    + *src2_g;
          *dest_b = (a * *src1_b) / 255L    + *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          a = 255L - *src2_alpha;
          *dest_r = (a * *src1_r) / 255L   ;
          *dest_g = (a * *src1_g) / 255L   ;
          *dest_b = (a * *src1_b) / 255L   ;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
          src2_alpha++;
     
        }
      break;
    default:
      break;
    } 
}

static void 
scanline_c_op_ca_rgb_u8(GeglFilter * filter, 
                        GeglTileIterator ** iters, 
                        gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint8 *dest_data[4];
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha;
  guint8 *src1_data[4];
  guint8 *src1_r, *src1_g, *src1_b, *src1_alpha;
  guint8 *src2_data[3];
  guint8 *src2_r, *src2_g, *src2_b;
  guint8 b; 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];
  src1_alpha = src1_data[3];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];


  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 255L;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OVER:
      while (width--)
        {
 
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
          *dest_alpha = 255L;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_IN:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
      while (width--)
        {
 
          b = 255L - *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
 
          b = *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
 
          b = 255L - *src1_alpha;
          *dest_r = (b * *src2_r) / 255L   ;
          *dest_g = (b * *src2_g) / 255L   ;
          *dest_b = (b * *src2_b) / 255L   ;
          *dest_alpha = b;
 
          dest_r++;
          dest_g++;
          dest_b++;
          dest_alpha++;

          src1_r++;
          src1_g++;
          src1_b++;
          src1_alpha++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    default:
      break;
    } 

}

static void 
scanline_c_op_c_rgb_u8 (GeglFilter * filter, 
                        GeglTileIterator ** iters, 
                        gint width)
{
  GeglCompPremult *self = GEGL_COMP_PREMULT(filter);
  guint8 *dest_data[3];
  guint8 *dest_r, *dest_g, *dest_b;
  guint8 *src1_data[3];
  guint8 *src1_r, *src1_g, *src1_b;
  guint8 *src2_data[3];
  guint8 *src2_r, *src2_g, *src2_b;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src1_data);
  gegl_tile_iterator_get_current (iters[2], (gpointer*)src2_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];

  src1_r = src1_data[0];
  src1_g = src1_data[1];
  src1_b = src1_data[2];

  src2_r = src2_data[0];
  src2_g = src2_data[1];
  src2_b = src2_data[2];

    
  switch(self->composite_mode)
    {
    case GEGL_COMPOSITE_MODE_REPLACE:
    case GEGL_COMPOSITE_MODE_OVER:
    case GEGL_COMPOSITE_MODE_IN:
    case GEGL_COMPOSITE_MODE_ATOP:
      while (width--)
        {
          *dest_r = *src2_r;
          *dest_g = *src2_g;
          *dest_b = *src2_b;
 
          dest_r++;
          dest_g++;
          dest_b++;

          src1_r++;
          src1_g++;
          src1_b++;

          src2_r++;
          src2_g++;
          src2_b++;
     
        }
      break;
    case GEGL_COMPOSITE_MODE_OUT:
    case GEGL_COMPOSITE_MODE_XOR:
      while (width--)
        {
          *dest_r = 0	;
          *dest_g = 0	;
          *dest_b = 0	;
 
          dest_r++;
          dest_g++;
          dest_b++;
     
        }
      break;
    default:
      break;
    }

}
