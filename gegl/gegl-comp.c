#include "gegl-comp.h"
#include "gegl-data.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include "gegl-image-buffer.h"
#include "gegl-image-buffer-iterator.h"
#include "gegl-image-buffer-data.h"
#include "gegl-param-specs.h"
#include "gegl-scalar-data.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_PREMULTIPLY,
  PROP_LAST 
};

static void class_init (GeglCompClass * klass);
static void init (GeglComp * self, GeglCompClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void prepare (GeglFilter * filter, GList * output_data_list, GList *input_data_list);

static gpointer parent_class = NULL;

GType
gegl_comp_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCompClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglComp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglComp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglCompClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_func = NULL;

  filter_class->prepare = prepare;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  /* op properties */
  gegl_op_class_install_input_data_property (op_class, 
                            gegl_param_spec_image_buffer("input-image-a", 
                                                       "InputImageA",
                                                       "InputImage A",
                                                       G_PARAM_PRIVATE));
  gegl_op_class_install_input_data_property (op_class, 
                            gegl_param_spec_image_buffer("input-image-b", 
                                                       "InputImageB",
                                                       "Input Image B",
                                                       G_PARAM_PRIVATE));
  gegl_op_class_install_input_data_property (op_class, 
                            g_param_spec_boolean ("premultiply",
                                                   "Premultiply",
                                                   "Premultiply the foreground.",
                                                   FALSE,
                                                   G_PARAM_PRIVATE));

  /* gobject properties */
  g_object_class_install_property (gobject_class, PROP_PREMULTIPLY,
                                   g_param_spec_boolean ("premultiply",
                                                         "Premultiply",
                                                         "Premultiply the foreground.",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT | 
                                                         G_PARAM_READWRITE));

}

static void 
init (GeglComp * self, 
      GeglCompClass * klass)
{
  gegl_op_add_input(GEGL_OP(self), GEGL_TYPE_IMAGE_BUFFER_DATA, "input-image-a", 0);
  gegl_op_add_input(GEGL_OP(self), GEGL_TYPE_IMAGE_BUFFER_DATA, "input-image-b", 1);
  gegl_op_add_input(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "premultiply", 2);

  self->premultiply = FALSE;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglComp *comp = GEGL_COMP (gobject);
  switch (prop_id)
  {
    case PROP_PREMULTIPLY:
      g_value_set_boolean(value, gegl_comp_get_premultiply(comp));  
      break;
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
  GeglComp *comp = GEGL_COMP (gobject);
  switch (prop_id)
  {
    case PROP_PREMULTIPLY:
      gegl_comp_set_premultiply(comp, g_value_get_boolean(value));  
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}


/**
 * gegl_comp_get_premultiply:
 * @self: a #GeglComp.
 *
 * Gets the premultiply flag.
 *
 * Returns: premultiply flag. 
 **/
gboolean
gegl_comp_get_premultiply (GeglComp * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_COMP (self), FALSE);

  return self->premultiply;
}


/**
 * gegl_comp_set_premultiply:
 * @self: a #GeglComp.
 * @premultiply: the premultiply flag. 
 *
 * Sets the premultiply flag. 
 *
 **/
void 
gegl_comp_set_premultiply (GeglComp * self, 
                           gboolean premultiply)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COMP (self));

  self->premultiply = premultiply;
}

static void 
prepare (GeglFilter * filter, 
         GList * output_data_list,
         GList * input_data_list)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglComp *self = GEGL_COMP(filter);

  GeglData *dest_data = g_list_nth_data(output_data_list, 0);
  GeglImageBuffer *dest = (GeglImageBuffer*)g_value_get_object(dest_data->value);
  GeglColorModel * dest_cm = gegl_image_buffer_get_color_model (dest);
  GeglColorSpace * dest_cs = gegl_color_model_color_space(dest_cm);
  GeglDataSpace * dest_ds = gegl_color_model_data_space(dest_cm);

  g_return_if_fail (dest_cm);

  {
    GeglDataSpaceType type = gegl_data_space_data_space_type(dest_ds);
    GeglColorSpaceType space = gegl_color_space_color_space_type(dest_cs);
    GeglCompClass *klass = GEGL_COMP_GET_CLASS(self);

    /* Get the appropriate scanline func from subclass */
    if(klass->get_scanline_func)
      point_op->scanline_processor->func = 
        (*klass->get_scanline_func)(self, space, type);

  }
}
