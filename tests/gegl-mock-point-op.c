#include "gegl-mock-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-buffer-iterator.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMockPointOpClass * klass);
static void init(GeglMockPointOp *self, GeglMockPointOpClass *klass);
static void prepare (GeglFilter * op, GList * output_params, GList * input_param);
static void finish (GeglFilter * op, GList * output_params, GList * input_param);
static void scanline (GeglFilter * op, GeglImageBufferIterator ** iters, gint width);

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
  gegl_node_add_input(GEGL_NODE(self), 0);
  gegl_node_add_input(GEGL_NODE(self), 1);
}

static void 
scanline (GeglFilter * op,
          GeglImageBufferIterator ** iters,
          gint width)
{
  LOG_DEBUG("scanline", "MockPointOp scanline was called");
}

static void 
prepare                (GeglFilter * op, 
                        GList * output_params,
                        GList * input_params)
{
  GeglPointOp *point_op = GEGL_POINT_OP(op); 
  point_op->scanline_processor->func = scanline;

  LOG_DEBUG("prepare", "input_params length is %d", g_list_length(input_params));
}

static void 
finish                (GeglFilter * op, 
                       GList * output_params,
                       GList * input_params)
{
  LOG_DEBUG("finish", "MockPointOp finish was called");
}
