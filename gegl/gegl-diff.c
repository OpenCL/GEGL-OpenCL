#include "gegl-diff.h"
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

static void class_init (GeglDiffClass * klass);
static void init (GeglDiff * self, GeglDiffClass * klass);

static void scanline (GeglFilter * op, GeglTileIterator ** iters, gint width);
static void prepare (GeglFilter * op, GList * output_attributes, GList *input_attributes);

static gpointer parent_class = NULL;

GType
gegl_diff_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDiffClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDiff),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglDiff", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglDiffClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;
}

static void 
init (GeglDiff * self, 
      GeglDiffClass * klass)
{
  g_object_set(self, "num_inputs", 2, NULL);
}

static void 
prepare (GeglFilter * op, 
         GList * output_attributes,
         GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(op); 

  GeglAttributes *dest_attributes = 
    (GeglAttributes*)g_list_nth_data(output_attributes, 0); 
  GeglTile *dest = (GeglTile *)g_value_get_object(dest_attributes->value);
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest);

  g_return_if_fail(dest_cm);

  /* Get correct scanline func for this color model */
  point_op->scanline_processor->func = scanline;        
}


/**
 * scanline:
 * @op: a #GeglFilter
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline.
 *
 * Processes a scanline.
 *
 **/
static void 
scanline (GeglFilter * op, 
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
 
      *dest_r = ABS (*src1_r - *src2_r);
      *dest_g = ABS (*src1_g - *src2_g);
      *dest_b = ABS (*src1_b - *src2_b);
 
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
