#include "gegl-unpremult.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color-model.h"
#include "gegl-attributes.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglUnpremultClass * klass);
static void init (GeglUnpremult * self, GeglUnpremultClass * klass);

static void prepare (GeglFilter * filter, GList * output_attributes, GList *input_attributes);
static void scanline (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_unpremult_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglUnpremultClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglUnpremult),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglUnpremult", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
init (GeglUnpremult * self, 
      GeglUnpremultClass * klass)
{
}

static void 
class_init (GeglUnpremultClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;
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
    gboolean dest_has_alpha = gegl_color_model_has_alpha(dest_cm);
    if(dest_has_alpha)
      point_op->scanline_processor->func = scanline;        
    else
      {
        point_op->scanline_processor->func = NULL;        
        g_warning("unpremult: prepare: dest has no alpha channel\n");
      } 
  }
}

/**
 * scanline:
 * @self_op: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  gfloat *dest_data[4];
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha;
  gfloat *src_data[4];
  gfloat *src_r, *src_g, *src_b, *src_alpha;

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  dest_alpha = dest_data[3];

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  src_alpha = src_data[3];
 

  while (width--)
    {
 
      if (*src_alpha == 1.0)
        {
          *dest_r = *src_r;
          *dest_g = *src_g;
          *dest_b = *src_b;
        }
      else if (*src_alpha == 0.0)
        {
          *dest_r = 0.0;
          *dest_g = 0.0;
          *dest_b = 0.0;
        }
      else
        {
          *dest_r = (*src_r * 1.0) / *src_alpha;
          *dest_g = (*src_g * 1.0) / *src_alpha;
          *dest_b = (*src_b * 1.0) / *src_alpha;
        }
 
        *dest_alpha = *src_alpha;
             
        dest_r++;
        dest_g++;
        dest_b++;
        dest_alpha++;

        src_r++;
        src_g++;
        src_b++;
        src_alpha++;

    }
}
