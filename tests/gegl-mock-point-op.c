#include "gegl-mock-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-param-specs.h"
#include "gegl-image-data.h"

enum
{
  PROP_0, 
  PROP_INPUT_IMAGE_A,
  PROP_INPUT_IMAGE_B,
  PROP_FADE_FLOAT,
  PROP_LAST 
};

static void class_init (GeglMockPointOpClass * klass);
static void init(GeglMockPointOp *self, GeglMockPointOpClass *klass);
static void prepare (GeglFilter * filter);
static void finish (GeglFilter * filter);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void scanline (GeglFilter * op, GeglImageIterator ** iters, gint width);

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
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;
  filter_class->finish = finish;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_INPUT_IMAGE_A,
               g_param_spec_object ("input-image-a",
                                    "InputImageA",
                                    "The input image a",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_INPUT_IMAGE_B,
               g_param_spec_object ("input-image-b",
                                    "InputImageB",
                                    "The input image b",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));
}

static void 
init (GeglMockPointOp * self, 
      GeglMockPointOpClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-a");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image-b");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglMockPointOp *self = GEGL_MOCK_POINT_OP(gobject);
  switch (prop_id)
  {
    case PROP_INPUT_IMAGE_A:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), input, 0);  
      }
      break;
    case PROP_INPUT_IMAGE_B:
      {
        GeglNode *input = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), input, 1);  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
scanline (GeglFilter * op,
          GeglImageIterator ** iters,
          gint width)
{
  LOG_DEBUG("scanline", "MockPointOp scanline was called");
}

static void 
prepare (GeglFilter * filter) 
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter); 
  point_op->scanline_processor->func = scanline;
}

static void 
finish (GeglFilter * filter) 
{
  LOG_DEBUG("finish", "MockPointOp finish was called");
}
