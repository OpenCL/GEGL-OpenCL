#include "gegl-mock-op.h"
#include "../gegl/gegl-value-types.h"
#include "../gegl/gegl-param-specs.h"

enum
{
  PROP_0, 
  PROP_SOURCE_0,
  PROP_SOURCE_1,
  PROP_SOURCE_2,
  PROP_M_SOURCE_0,
  PROP_M_SOURCE_1,
  PROP_M_SOURCE_2,
  PROP_LAST 
};

static void class_init        (GeglMockOpClass * klass);
static void init (GeglMockOp * self, GeglMockOpClass * klass);
static void finalize (GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gpointer parent_class = NULL;

GType
gegl_mock_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OP, 
                                     "GeglMockOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
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

  g_object_class_install_property (gobject_class, PROP_SOURCE_2,
               g_param_spec_object ("source-2",
                                    "Source2",
                                    "The source 2",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_SOURCE_0,
             gegl_param_spec_m_source ("m-source-0",
                                        "MSource0",
                                        "Multi source 0",
                                        G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_SOURCE_1,
             gegl_param_spec_m_source ("m-source-1",
                                        "MSource1",
                                        "Multi source 1",
                                        G_PARAM_WRITABLE));


  g_object_class_install_property (gobject_class, PROP_M_SOURCE_2,
             gegl_param_spec_m_source ("m-source-2",
                                        "MSource2",
                                        "Multi source 2",
                                        G_PARAM_WRITABLE));
}

static void 
init (GeglMockOp * self, 
      GeglMockOpClass * klass)
{
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockOp *self = GEGL_MOCK_OP(gobject);
  switch (prop_id)
  {
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
  GeglMockOp *self = GEGL_MOCK_OP(gobject);
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
    case PROP_SOURCE_2:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gegl_node_set_source(GEGL_NODE(self), source, 2);  
      }
      break;
    case PROP_M_SOURCE_0:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gegl_node_set_m_source(GEGL_NODE(self), source, 0, output);  
      }
      break;
    case PROP_M_SOURCE_1:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gegl_node_set_m_source(GEGL_NODE(self), source, 1, output);  
      }
      break;
    case PROP_M_SOURCE_2:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gegl_node_set_m_source(GEGL_NODE(self), source, 2, output);  
      }
      break;
    default:
      break;
  }
}
