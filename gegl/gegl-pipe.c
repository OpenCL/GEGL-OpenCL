#include "gegl-pipe.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include "gegl-image-buffer.h"
#include "gegl-image-buffer-iterator.h"
#include "gegl-image-buffer-data.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglPipeClass * klass);
static void init (GeglPipe * self, GeglPipeClass * klass);
static void finalize(GObject * gobject);

static void process (GeglFilter * filter, GList * output_data_list, GList * input_data_list);
static void prepare (GeglFilter * filter, GList * output_data_list, GList * input_data_list);
static gpointer parent_class = NULL;

static void validate_inputs  (GeglFilter *filter, GList *data_list);
static void validate_outputs (GeglFilter *filter, GList *output_data_list);

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

      type = g_type_register_static (GEGL_TYPE_IMAGE, 
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

  /* op properties */
  gegl_op_class_install_input_data_property(op_class, 
                                       gegl_param_spec_image_buffer("input-image", 
                                                                  "InputImage",
                                                                  "The input image.",
                                                                   G_PARAM_PRIVATE));
}

static void 
init (GeglPipe * self, 
      GeglPipeClass * klass)
{
  gegl_op_add_input(GEGL_OP(self), GEGL_TYPE_IMAGE_BUFFER_DATA, "input-image", 0);

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
                 GList *data_list)
{
}

static void 
validate_outputs (GeglFilter *filter,
                  GList *output_data_list)
{
  GeglData *data = g_list_nth_data(output_data_list, 0);

  if(G_VALUE_TYPE(data->value) != GEGL_TYPE_IMAGE_BUFFER)
    g_value_init(data->value, GEGL_TYPE_IMAGE_BUFFER);
}

static void 
prepare (GeglFilter * filter, 
         GList * output_data_list,
         GList * input_data_list)
{
  GeglPipe *self = GEGL_PIPE(filter);
  GeglData *src_data = (GeglData*)g_list_nth_data(input_data_list, 0); 
  GeglData *dest_data = (GeglData*)g_list_nth_data(output_data_list, 0); 
  GeglImageBuffer *src = (GeglImageBuffer*)g_value_get_object(src_data->value);
  GeglColorModel * src_cm = gegl_image_buffer_get_color_model (src);
  GeglColorSpace * src_cs = gegl_color_model_color_space(src_cm);
  GeglDataSpace * src_ds = gegl_color_model_data_space(src_cm);
  GeglDataSpaceType type = gegl_data_space_data_space_type(src_ds);
  GeglColorSpaceType space = gegl_color_space_color_space_type(src_cs);
  GeglPipeClass *klass = GEGL_PIPE_GET_CLASS(self);

  /* copy the src value to the dest value */
  g_value_set_object(dest_data->value, src);

  /* Get the appropriate scanline func from subclass */
  if(klass->get_scanline_func)
    self->scanline_processor->func = (*klass->get_scanline_func)(self, space, type);
}

static void 
process (GeglFilter * filter, 
         GList * output_data_list,
         GList * input_data_list)
{
  GeglPipe *self =  GEGL_PIPE(filter);
  gegl_scanline_processor_process(self->scanline_processor, 
                                  output_data_list,
                                  input_data_list);
}
