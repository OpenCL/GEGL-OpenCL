#include "config.h"

#include "gegl-comp.h"
#include "gegl-image-data.h"
#include "gegl-scalar-data.h"
#include "gegl-image.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"

enum
{
  PROP_0,
  PROP_BACKGROUND,
  PROP_FOREGROUND,
  PROP_M_BACKGROUND,
  PROP_M_FOREGROUND,
  PROP_PREMULTIPLY,
  PROP_LAST
};

static void class_init      (GeglCompClass *klass);
static void init            (GeglComp      *self,
                             GeglCompClass *klass);
static void get_property    (GObject       *gobject,
                             guint          prop_id,
                             GValue        *value,
                             GParamSpec    *pspec);
static void set_property    (GObject       *gobject,
                             guint          prop_id,
                             const GValue  *value,
                             GParamSpec    *pspec);
static void validate_inputs (GeglFilter    *filter,
                             GArray        *collected_data);
static void prepare         (GeglFilter    *filter);


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
        NULL
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
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  klass->get_scanline_function = NULL;

  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_BACKGROUND,
               g_param_spec_object ("background",
                                    "Background",
                                    "The background",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_FOREGROUND,
               g_param_spec_object ("foreground",
                                    "Foreground",
                                    "The foreground",
                                     GEGL_TYPE_OP,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_BACKGROUND,
             gegl_param_spec_m_source ("m-background",
                                        "MBackground",
                                        "The m-background",
                                        G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_M_FOREGROUND,
             gegl_param_spec_m_source ("m-foreground",
                                        "MForeground",
                                        "The m-foreground",
                                        G_PARAM_WRITABLE));

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
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "background");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "foreground");
  gegl_op_add_input_data(GEGL_OP(self), GEGL_TYPE_SCALAR_DATA, "premultiply");
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
      {
        GValue *data_value = gegl_op_get_input_data_value(GEGL_OP(comp), "premultiply");
        g_param_value_convert(pspec, data_value, value, TRUE);
      }
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
    case PROP_BACKGROUND:
      {
        GeglNode *background = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(comp), "background");
        gegl_node_set_source(GEGL_NODE(comp), background, index);
      }
      break;
    case PROP_FOREGROUND:
      {
        GeglNode *foreground = (GeglNode*)g_value_get_object(value);
        gint index = gegl_op_get_input_data_index(GEGL_OP(comp), "foreground");
        gegl_node_set_source(GEGL_NODE(comp), foreground, index);
      }
      break;
    case PROP_M_BACKGROUND:
      {
        gint output;
        GeglNode *background = g_value_get_m_source(value, &output);
        gint index = gegl_op_get_input_data_index(GEGL_OP(comp), "background");
        gegl_node_set_m_source(GEGL_NODE(comp), background, index, output);
      }
      break;
    case PROP_M_FOREGROUND:
      {
        gint output;
        GeglNode *foreground = g_value_get_m_source(value, &output);
        gint index = gegl_op_get_input_data_index(GEGL_OP(comp), "foreground");
        gegl_node_set_m_source(GEGL_NODE(comp), foreground, index, output);
      }
      break;
    case PROP_PREMULTIPLY:
      gegl_op_set_input_data_value(GEGL_OP(comp), "premultiply", value);
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
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "background");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "background", value);
  }

  {
    gint index = gegl_op_get_input_data_index(GEGL_OP(filter), "foreground");
    GeglData * data = g_array_index(collected_data, GeglData*, index);
    GValue *value = gegl_data_get_value(data);
    gegl_op_set_input_data_value(GEGL_OP(filter), "foreground", value);
  }
}


static void
prepare (GeglFilter * filter)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter);
  GeglComp *self = GEGL_COMP(filter);

  GValue *dest_value = gegl_op_get_output_data_value(GEGL_OP(filter), "dest");
  GeglImage *dest = (GeglImage*)g_value_get_object(dest_value);
  GeglColorModel * dest_cm = gegl_image_get_color_model (dest);
  //GeglColorSpace * dest_color_space = gegl_color_model_color_space(dest_cm);
  GeglCompClass *klass = GEGL_COMP_GET_CLASS(self);

  /* Get the appropriate scanline func from subclass */
  if(klass->get_scanline_function)
    point_op->scanline_processor->func =
        klass->get_scanline_function(self, dest_cm);
  else
    g_print("Cant find scanline func\n");
}
