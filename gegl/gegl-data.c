#include "gegl-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_PARAM_SPEC,
  PROP_LAST 
};

static void class_init (GeglDataClass * klass);
static void init (GeglData *self, GeglDataClass * klass);
static void finalize(GObject * gobject);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglDataClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_PARAM_SPEC,
                                   g_param_spec_param ("param-spec",
                                                       "ParamSpec",
                                                       "The Param's param spec.",
                                                        G_TYPE_PARAM,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void 
init (GeglData * self, 
      GeglDataClass * klass)
{
  self->value = g_new0(GValue, 1); 
  self->param_spec = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglData *self = GEGL_DATA(gobject);

  if(G_IS_VALUE(self->value))
    {
      g_value_unset(self->value);
      g_free(self->value);
    }

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglData *self = GEGL_DATA(gobject);
  switch (prop_id)
  {
    case PROP_PARAM_SPEC:
      g_value_set_param (value, gegl_data_get_param_spec(self));
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
  GeglData *self = GEGL_DATA(gobject);
  switch (prop_id)
  {
    case PROP_PARAM_SPEC:
      gegl_data_set_param_spec(self,g_value_get_param (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

/**
 * gegl_data_get_value:
 * @self: a #GeglData
 *
 * Gets the GValue for this data.  
 *
 * Returns: the GValue for the data. 
 **/
GValue * 
gegl_data_get_value (GeglData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_DATA (self), NULL);
   
  return self->value;
}

GParamSpec * 
gegl_data_get_param_spec (GeglData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_DATA (self), NULL);
   
  return self->param_spec;
}

void
gegl_data_set_param_spec (GeglData * self,
                          GParamSpec *param_spec)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_DATA (self));
   
  self->param_spec = param_spec;
}
