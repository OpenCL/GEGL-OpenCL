#include "gegl-pipe.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include "gegl-image-data.h"
#include "gegl-image-data-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglPipeClass * klass);
static void init (GeglPipe * self, GeglPipeClass * klass);
static void finalize(GObject * gobject);

static void process (GeglFilter * filter, GList * attributes, GList * input_attributes);
static void prepare (GeglFilter * filter, GList * attributes, GList * input_attributes);
static gpointer parent_class = NULL;

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

      type = g_type_register_static (GEGL_TYPE_FILTER , 
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

  filter_class->process = process;
  filter_class->prepare = prepare;

  return;
}

static void 
init (GeglPipe * self, 
      GeglPipeClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op = GEGL_FILTER(self);

  g_object_set(self, "num_outputs", 1, NULL);
  g_object_set(self, "num_inputs", 1, NULL);

  return;
}

static void
finalize(GObject *gobject)
{
  GeglPipe * self_pipe = GEGL_PIPE(gobject);
  g_object_unref(self_pipe->scanline_processor);
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
prepare (GeglFilter * filter, 
         GList * attributes,
         GList * input_attributes)
{
  GeglPipe *self = GEGL_PIPE(filter);

  GeglAttributes *src_attributes = (GeglAttributes*)g_list_nth_data(input_attributes, 0); 
  GeglImageData *src = (GeglImageData*)g_value_get_object(src_attributes->value);
  GeglColorModel *src_cm = gegl_image_data_get_color_model(src);
  GeglColorSpace * src_cs = gegl_color_model_color_space(src_cm);
  GeglDataSpace * src_ds = gegl_color_model_data_space(src_cm);

  GeglAttributes *dest_attributes = (GeglAttributes*)g_list_nth_data(attributes, 0); 
  g_value_set_object(dest_attributes->value, src);

  {
    GeglDataSpaceType type = gegl_data_space_data_space_type(src_ds);
    GeglColorSpaceType space = gegl_color_space_color_space_type(src_cs);
    GeglPipeClass *klass = GEGL_PIPE_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      self->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}

static void 
process (GeglFilter * filter, 
         GList * attributes,
         GList * input_attributes)
{
  GeglPipe *self =  GEGL_PIPE(filter);
  gegl_scanline_processor_process(self->scanline_processor, 
                                  attributes,
                                  input_attributes);
}
