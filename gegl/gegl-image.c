#include "gegl-image.h"
#include "gegl-op.h"
#include "gegl-tile.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

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

static void compute_derived_color_model (GeglOp * op, GList * input_values);

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

      type = g_type_register_static (GEGL_TYPE_OP , 
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
  GeglOpClass *op_class = GEGL_OP_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  op_class->compute_derived_color_model = compute_derived_color_model;

  g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                   g_param_spec_object ("colormodel",
                                                        "ColorModel",
                                                        "The GeglImage's colormodel",
                                                         GEGL_TYPE_COLOR_MODEL,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglImage * self, 
      GeglImageClass * klass)
{
  GeglOp *op = GEGL_OP(self);
  GValue * output_value;

  gegl_op_set_num_output_values(op, 1);
  output_value = gegl_op_get_nth_output_value(op,0);
  g_value_init(output_value, GEGL_TYPE_IMAGE_DATA);

  self->color_model = NULL;
  self->derived_color_model = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglImage *self = GEGL_IMAGE(gobject);

  if(self->color_model)
    g_object_unref(self->color_model);

  if(self->tile)
    g_object_unref(self->tile);

  if(self->derived_color_model)
    g_object_unref(self->derived_color_model);

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

  /* Unref any old one */
  if(self->color_model)
    g_object_unref(self->color_model);

  /* Set the new color model */
  self->color_model = cm;

  if(cm)
    g_object_ref(cm);
}

static void 
compute_derived_color_model (GeglOp * op, 
                             GList * input_values)
{
  gint num_inputs = g_list_length(input_values);

  if(num_inputs > 0)
    {
      GValue * input_value = g_list_nth_data(input_values, 0);

      if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
        {
          GeglTile * tile =  g_value_get_image_data_tile(input_value);
          GeglColorModel *cm = gegl_tile_get_color_model(tile);
          gegl_image_set_derived_color_model(GEGL_IMAGE(op), cm);
        }
    }
}

void 
gegl_image_set_derived_color_model (GeglImage * self, 
                                    GeglColorModel * cm)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE (self));

  /* Unref any old one */
  if(self->derived_color_model)
    g_object_unref(self->derived_color_model);

  /* Set the new color model */
  self->derived_color_model = cm;

  if(self->derived_color_model)
    g_object_ref(self->derived_color_model);
}
