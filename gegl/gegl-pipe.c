#include "gegl-pipe.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-channel-space.h"
#include "gegl-image.h"
#include "gegl-image-iterator.h"
#include "gegl-image-data.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"

static void class_init (GeglPipeClass * klass);
static void init (GeglPipe * self, GeglPipeClass * klass);
static void finalize(GObject * gobject);

static void process (GeglFilter * filter, GList * data_outputs, GList * data_inputs);
static void prepare (GeglFilter * filter, GList * data_outputs, GList * data_inputs);
static gpointer parent_class = NULL;

static void validate_inputs  (GeglFilter *filter, GList *data_inputs);
static void validate_outputs (GeglFilter *filter, GList *data_outputs);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  filter_class->process = process;
  filter_class->prepare = prepare;

  filter_class->validate_inputs = validate_inputs;
  filter_class->validate_outputs = validate_outputs;

  /* data properties */
  gegl_op_class_install_data_input_property(op_class, 
                         gegl_param_spec_image("input-image", 
                                               "InputImage",
                                               "The input image.",
                                                G_PARAM_PRIVATE));
}

static void 
init (GeglPipe * self, 
      GeglPipeClass * klass)
{
  gegl_op_append_input(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "input-image");

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
validate_inputs (GeglFilter *filter,
                 GList *data_inputs)
{
}

static void 
validate_outputs (GeglFilter *filter,
                  GList *data_outputs)
{
  GeglData *data = g_list_nth_data(data_outputs, 0);
  GValue *value = gegl_data_get_value(data);

  if(G_VALUE_TYPE(value) != GEGL_TYPE_IMAGE)
    g_value_init(value, GEGL_TYPE_IMAGE);
}

static void 
prepare (GeglFilter * filter, 
         GList * data_outputs,
         GList * data_inputs)
{
  GeglPipe *self = GEGL_PIPE(filter);
  GeglData *src_data = (GeglData*)g_list_nth_data (data_inputs, 0); 
  GValue *src_value = gegl_data_get_value (src_data);
  GeglData *dest_data = (GeglData*)g_list_nth_data (data_outputs, 0); 
  GValue *dest_value = gegl_data_get_value (dest_data);
  GeglImage *src = (GeglImage*)g_value_get_object (src_value);
  GeglColorModel * src_cm = gegl_image_get_color_model (src);
  GeglColorSpace * src_color_space = gegl_color_model_color_space (src_cm);
  GeglChannelSpace * src_channel_space = gegl_color_model_channel_space (src_cm);
  GeglChannelSpaceType channel_type = gegl_channel_space_channel_space_type (src_channel_space);
  GeglColorSpaceType color_type = gegl_color_space_color_space_type (src_color_space);
  GeglPipeClass *klass = GEGL_PIPE_GET_CLASS(self);

  /* copy the src value to the dest value */
  g_value_set_object(dest_value, src);

  /* Get the appropriate scanline func from subclass */
  if(klass->get_scanline_func)
    self->scanline_processor->func = (*klass->get_scanline_func)(self, 
                                                                 color_type, 
                                                                 channel_type);
}

static void 
process (GeglFilter * filter, 
         GList * data_outputs,
         GList * data_inputs)
{
  GeglPipe *self =  GEGL_PIPE(filter);
  gegl_scanline_processor_process(self->scanline_processor, 
                                  data_outputs,
                                  data_inputs);
}
