#include "gegl-mock-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile-iterator.h"
#include "gegl-attributes.h"

enum
{
  PROP_0, 
  PROP_INPUT0,
  PROP_INPUT1,
  PROP_LAST 
};

static void class_init (GeglMockPointOpClass * klass);
static void init(GeglMockPointOp *self, GeglMockPointOpClass *klass);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
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
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  parent_class = g_type_class_peek_parent(klass);

  filter_class->prepare = prepare;
  filter_class->finish = finish;

  g_object_class_install_property (gobject_class, PROP_INPUT0,
                                   g_param_spec_object ("input0",
                                                        "Input0",
                                                        "Input 0 of this node",
                                                         GEGL_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_INPUT1,
                                   g_param_spec_object ("input1",
                                                        "Input1",
                                                        "Input 1 of this node",
                                                         GEGL_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));


  return;
}

static void 
init (GeglMockPointOp * self, 
      GeglMockPointOpClass * klass)
{
  GeglNode * node = GEGL_NODE(self);
  gegl_node_set_num_inputs(node,2);
  return;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUT0:
      {
        gegl_node_set_nth_input(node, (GeglNode*)g_value_get_object(value), 0);  
      }
      break;
    case PROP_INPUT1:
      {
        gegl_node_set_nth_input(node, (GeglNode*)g_value_get_object(value), 1);  
      }
      break;
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUT0:
      {
        g_value_set_object(value, (GObject*)gegl_node_get_nth_input(node, 0));  
      }
      break;
    case PROP_INPUT1:
      {
        g_value_set_object(value, (GObject*)gegl_node_get_nth_input(node, 1));  
      }
    default:
      break;
  }
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
