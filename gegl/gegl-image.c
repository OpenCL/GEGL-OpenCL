#include "gegl-image.h"
#include "gegl-image-buffer.h"
#include "gegl-image-buffer-data.h"
#include "gegl-data.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-tile.h"

enum
{
  PROP_0, 
  PROP_COLOR_MODEL, 
  PROP_LAST 
};

static void class_init (GeglImageClass * klass);
static void init (GeglImage * self, GeglImageClass * klass);
static void finalize(GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void validate_outputs (GeglFilter *filter, GList *output_data_list);
static GeglColorModel *compute_derived_color_model (GeglFilter * filter, GList* input_color_models);

static gpointer parent_class = NULL;

GType
gegl_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglImage", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglImageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->compute_derived_color_model = compute_derived_color_model;
  filter_class->validate_outputs = validate_outputs;

  /* op properties */
  gegl_op_class_install_output_data_property(op_class, 
                                        gegl_param_spec_image_buffer("output-image", 
                                                                    "OutputImage",
                                                                    "Image output",
                                                                    G_PARAM_PRIVATE));
  /* gobject properties */
  g_object_class_install_property (gobject_class, 
                                   PROP_COLOR_MODEL, 
                                   g_param_spec_object ("colormodel",
                                                        "ColorModel",
                                                        "The GeglImage's colormodel",
                                                        GEGL_TYPE_COLOR_MODEL,
                                                        G_PARAM_CONSTRUCT | 
                                                        G_PARAM_READWRITE));
}

static void 
init (GeglImage * self, 
      GeglImageClass * klass)
{
  gegl_op_add_output(GEGL_OP(self), GEGL_TYPE_IMAGE_BUFFER_DATA, "output-image", 0);

  self->image_buffer = g_object_new(GEGL_TYPE_IMAGE_BUFFER, NULL);
  self->color_model = NULL;
  self->derived_color_model = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglImage *self = GEGL_IMAGE(gobject);

  if(self->image_buffer)
    g_object_unref(self->image_buffer);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglImage *self = GEGL_IMAGE(gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          GeglColorModel *cm = (GeglColorModel*)(g_value_get_object(value)); 
          gegl_image_set_color_model (self,cm);
        }
      else
        g_message("Cant set colormodel after construction");
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
  GeglImage *self = GEGL_IMAGE (gobject);

  switch (prop_id)
  {
    case PROP_COLOR_MODEL:
      g_value_set_object (value, (GObject*)gegl_image_color_model(self));
      break;
    default:
      break;
  }
}

void
gegl_image_set_image_buffer (GeglImage * self,
                     GeglImageBuffer *image_buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  if(image_buffer)
   g_object_ref(image_buffer);

  if(self->image_buffer)
   g_object_unref(self->image_buffer);

  self->image_buffer = image_buffer;
}

GeglImageBuffer *
gegl_image_get_image_buffer (GeglImage * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE (self), NULL);

  return self->image_buffer;
}

/**
 * gegl_image_color_model:
 * @self: a #GeglImage.
 *
 * Gets the color model of this image.
 *
 * Returns: the #GeglColorModel, or NULL. 
 **/
GeglColorModel * 
gegl_image_color_model (GeglImage * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE (self), NULL);

  return self->color_model ? 
         self->color_model :
         self->derived_color_model;
}

/**
 * gegl_image_set_color_model:
 * @self: a #GeglImage.
 * @cm: a #GeglColorModel.
 *
 * Sets the color model explicitly for this image. This takes precedence over
 * derived color models during graph evaluation. Set this when you want to
 * force an image to have a particular color model.
 *
 **/
void 
gegl_image_set_color_model (GeglImage * self, 
                            GeglColorModel * cm)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  /* Set the new color model */
  self->color_model = cm;
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                           GList * input_color_models)
{
  GeglColorModel *cm = (GeglColorModel *)g_list_nth_data(input_color_models, 0);
  gegl_image_set_derived_color_model(GEGL_IMAGE(filter), cm);
  return cm; 
}


static void 
validate_outputs (GeglFilter *filter,
                  GList *output_data_list)
{
  GeglData *data = g_list_nth_data(output_data_list, 0);
  GeglImage *self = GEGL_IMAGE(filter);
  GeglImageBuffer *image_buffer;

  if(G_VALUE_TYPE(data->value) != GEGL_TYPE_IMAGE_BUFFER)
    g_value_init(data->value, GEGL_TYPE_IMAGE_BUFFER);

  image_buffer = g_value_get_object(data->value);
  if(!image_buffer)
    {
      GeglImageBufferData *image_buffer_data = GEGL_IMAGE_BUFFER_DATA(data);
      gegl_image_buffer_create_tile(self->image_buffer, 
                                  image_buffer_data->color_model, 
                                  &image_buffer_data->rect); 

      g_value_set_object(data->value, self->image_buffer);
    }
}

/**
 * gegl_image_set_derived_model:
 * @self: a #GeglImage.
 * @cm: a #GeglColorModel.
 *
 * Set the derived color model.
 *
 **/
void 
gegl_image_set_derived_color_model (GeglImage * self, 
                                    GeglColorModel * cm)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  /* Set the new color model */
  self->derived_color_model = cm;
}
