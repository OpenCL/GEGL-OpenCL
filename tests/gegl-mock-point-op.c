#include "gegl-mock-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-param-specs.h"
#include "gegl-image-data.h"

enum
{
  PROP_0, 
  PROP_SOURCE_0,
  PROP_SOURCE_1,
  PROP_LAST 
};

static void class_init (GeglMockPointOpClass * klass);
static void init(GeglMockPointOp *self, GeglMockPointOpClass *klass);
static void prepare (GeglFilter * filter);
static void finish (GeglFilter * filter);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void scanline (GeglFilter * op, GeglScanlineProcessor *processor, gint width);

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
        NULL
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

  g_object_class_install_property (gobject_class, PROP_SOURCE_0,
               g_param_spec_object ("source-0",
                                    "Source0",
                                    "The source 0",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_SOURCE_1,
               g_param_spec_object ("source-1",
                                    "Source1",
                                    "The source 1",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));
}

static void 
init (GeglMockPointOp * self, 
      GeglMockPointOpClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source-0");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source-1");
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
    case PROP_SOURCE_0:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), source, 0);  
      }
      break;
    case PROP_SOURCE_1:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), source, 1);  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
scanline (GeglFilter * op,
          GeglScanlineProcessor *processor,
          gint width)
{
  gegl_log_debug(__FILE__, __LINE__,"scanline", "MockPointOp scanline was called");
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
  gegl_log_debug(__FILE__, __LINE__,"finish", "MockPointOp finish was called");
}
