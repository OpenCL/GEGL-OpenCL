#include "gegl-no-input.h"
#include "gegl-data.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include "gegl-image-buffer.h"
#include "gegl-image-buffer-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglNoInputClass * klass);
static void init (GeglNoInput * self, GeglNoInputClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void compute_have_rect(GeglFilter * filter, GeglRect *have_rect, GList * input_have_rects);

static void prepare (GeglFilter * filter, GList *output_data_list, GList *input_data_list);

static gpointer parent_class = NULL;

GType
gegl_no_input_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglNoInputClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglNoInput),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglNoInput", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglNoInputClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;
  filter_class->compute_have_rect = compute_have_rect;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
}

static void 
init (GeglNoInput * self, 
      GeglNoInputClass * klass)
{
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
  switch (prop_id)
  {
    default:
      break;
  }
}

static void 
compute_have_rect(GeglFilter * op, 
                  GeglRect *have_rect, 
                  GList * input_have_rects)
{ 
  gegl_rect_set(have_rect, 0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT);
}

static void 
prepare (GeglFilter * filter, 
         GList * output_data_list,
         GList * input_data_list)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglNoInput *self = GEGL_NO_INPUT(filter);

  GeglData *dest_data = g_list_nth_data(output_data_list, 0);
  GeglImageBuffer *dest = (GeglImageBuffer*)g_value_get_object(dest_data->value);
  GeglColorModel * dest_cm = gegl_image_buffer_get_color_model (dest);
  GeglColorSpace * dest_cs = gegl_color_model_color_space(dest_cm);
  GeglDataSpace * dest_ds = gegl_color_model_data_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglDataSpaceType type = gegl_data_space_data_space_type(dest_ds);
    GeglColorSpaceType space = gegl_color_space_color_space_type(dest_cs);
    GeglNoInputClass *klass = GEGL_NO_INPUT_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
