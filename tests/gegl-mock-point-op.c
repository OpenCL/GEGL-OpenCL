#include "gegl-mock-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile-iterator.h"
#include "gegl-attributes.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMockPointOpClass * klass);
static void init(GeglMockPointOp *self, GeglMockPointOpClass *klass);
static void prepare (GeglFilter * op, GList * output_attributes, GList * input_attributes);
static void finish (GeglFilter * op, GList * output_attributes, GList * input_attributes);
static void scanline (GeglFilter * op, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_mock_point_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockPointOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockPointOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP, 
                                     "GeglMockPointOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockPointOpClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;
  filter_class->finish = finish;
}

static void 
init (GeglMockPointOp * self, 
      GeglMockPointOpClass * klass)
{
  g_object_set(self, "num_inputs", 2, NULL);
}

static void 
scanline (GeglFilter * op,
          GeglTileIterator ** iters,
          gint width)
{
  LOG_DEBUG("scanline", "MockPointOp scanline was called");
}

static void 
prepare                (GeglFilter * op, 
                        GList * output_attributes,
                        GList * input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(op); 
  point_op->scanline_processor->func = scanline;

  LOG_DEBUG("prepare", "input_attributes length is %d", g_list_length(input_attributes));
}

static void 
finish                (GeglFilter * op, 
                       GList * output_attributes,
                       GList * input_attributes)
{
  LOG_DEBUG("finish", "MockPointOp finish was called");
}
