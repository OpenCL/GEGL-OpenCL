#include "gegl-pipe.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-image.h"
#include "gegl-image-iterator.h"
#include "gegl-image-data.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"

enum
{
  PROP_0, 
  PROP_SOURCE,
  PROP_LAST 
};

static void class_init (GeglPipeClass * klass);
static void init (GeglPipe * self, GeglPipeClass * klass);
static void finalize(GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void process (GeglFilter * filter);
static void prepare (GeglFilter * filter);
static gpointer parent_class = NULL;

static void validate_inputs  (GeglFilter *filter, GArray *collected_data);
static void validate_outputs (GeglFilter *filter);

GType
gegl_pipe_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPipeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPipe),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE_OP, 
                                     "GeglPipe", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPipeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->process = process;
  filter_class->prepare = prepare;

  filter_class->validate_inputs = validate_inputs;
  filter_class->validate_outputs = validate_outputs;

  g_object_class_install_property (gobject_class, PROP_SOURCE,
               g_param_spec_object ("source",
                                    "Source",
                                    "The source image",
                                     GEGL_TYPE_OP,
                                     G_PARAM_CONSTRUCT_ONLY | 
                                     G_PARAM_WRITABLE));
}


static void 
init (GeglPipe * self, 
      GeglPipeClass * klass)
{
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "source");

  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op = GEGL_FILTER(self);
}

static void
finalize(GObject *gobject)
{
  GeglPipe * self = GEGL_PIPE(gobject);

  g_object_unref(self->scanline_processor);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
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
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglPipe *pipe = GEGL_PIPE (gobject);
  switch (prop_id)
  {
    case PROP_SOURCE:
      {
        GeglNode *source = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(pipe), "source");
        gegl_node_set_source(GEGL_NODE(pipe), source, index);  
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void 
validate_inputs  (GeglFilter *filter, 
                        GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "source");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "source", value);
  }
}

static void 
validate_outputs (GeglFilter *filter)
{
  GValue *value = gegl_op_get_output_data_value(GEGL_OP(filter), "dest");

  if(G_VALUE_TYPE(value) != GEGL_TYPE_IMAGE)
    g_value_init(value, GEGL_TYPE_IMAGE);
}

static void 
prepare (GeglFilter * filter)
{
  GeglPipe *self = GEGL_PIPE(filter);
  GValue *dest_value = gegl_op_get_output_data_value(GEGL_OP(filter), "dest");
  GValue *src_value = gegl_op_get_input_data_value(GEGL_OP(filter), "source");
  GeglImage *src = (GeglImage*)g_value_get_object (src_value);
  GeglColorModel * src_cm = gegl_image_get_color_model (src);
  GeglPipeClass *klass = GEGL_PIPE_GET_CLASS(self);

  /* copy the src value to the dest value */
  g_value_set_object(dest_value, src);

  /* Get the appropriate scanline func from subclass */
  if(klass->get_scanline_function)
    self->scanline_processor->func = 
        klass->get_scanline_function(self, src_cm);
  else
    g_print("Cant find scanline func\n");
}

static void 
process (GeglFilter * filter) 
{
  GeglPipe *self =  GEGL_PIPE(filter);
  gegl_scanline_processor_process(self->scanline_processor);
}
