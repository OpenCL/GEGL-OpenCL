#include "gegl-binary.h"
#include "gegl-image-data.h"
#include "gegl-scalar-data.h"
#include "gegl-color-model.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-image.h"

enum
{
  PROP_0, 
  PROP_SOURCE_0,
  PROP_SOURCE_1,
  PROP_M_SOURCE_0,
  PROP_M_SOURCE_1,
  PROP_FADE,
  PROP_LAST 
};

static void class_init (GeglBinaryClass * klass);
static void init (GeglBinary * self, GeglBinaryClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void validate_inputs (GeglFilter *filter, GArray *collected_data);

static void prepare (GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_binary_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBinaryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBinary),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglBinary", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglBinaryClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_function = NULL;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

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

  g_object_class_install_property (gobject_class, PROP_M_SOURCE_0,
             gegl_param_spec_m_source ("m-source-0",
                                        "MSource0",
                                        "The m-source 0",
                                        G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_SOURCE_1,
             gegl_param_spec_m_source ("m-source-1",
                                        "MSource1",
                                        "The m-source 1",
                                        G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class, PROP_FADE, 
              g_param_spec_float ("fade",
                                  "Fade",
                                  "Fade on B.",
                                  0.0, 
                                  G_MAXFLOAT,
                                  1.0,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE));
}

static void 
init (GeglBinary * self, 
      GeglBinaryClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source-0");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source-1");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "fade");
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglBinary *self = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_FADE:
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(self), "fade");
        g_value_set_float(value, g_value_get_float(data_value));  
      }
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
  GeglBinary *self = GEGL_BINARY(gobject);
  switch (prop_id)
  {
    case PROP_SOURCE_0:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source-0");
        gegl_node_set_source(GEGL_NODE(self), source, index);  
      }
      break;
    case PROP_SOURCE_1:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source-1");
        gegl_node_set_source(GEGL_NODE(self), source, index);  
      }
      break;
    case PROP_M_SOURCE_0:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source-0");
        gegl_node_set_m_source(GEGL_NODE(self), source, index, output);  
      }
      break;
    case PROP_M_SOURCE_1:
      {
        gint output;
        GeglNode *source = g_value_get_m_source(value, &output);
        gint index = gegl_op_get_input_data_index(GEGL_OP(self), "source-1");
        gegl_node_set_m_source(GEGL_NODE(self), source, index, output);  
      }
      break;
    case PROP_FADE:
      gegl_op_set_input_data_value(GEGL_OP(self), "fade", value);
      break;
    default:
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                        GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "source-0");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "source-0", value);
  }

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "source-1");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "source-1", value);
  }
}

static void 
prepare (GeglFilter * filter) 
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglBinary *self = GEGL_BINARY(filter);
  GValue *dest_value = gegl_op_get_output_data_value(GEGL_OP(filter), "dest");
  GeglImage *dest = (GeglImage*)g_value_get_object(dest_value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  GeglBinaryClass *klass = GEGL_BINARY_GET_CLASS(self);

  if(klass->get_scanline_function)
    point_op->scanline_processor->func = 
      klass->get_scanline_function(self, dest_cm);
  else
    g_print("Cant find scanline func\n");
}
