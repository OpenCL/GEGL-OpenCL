#include "gegl-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_DATA_NAME,
  PROP_LAST 
};

static void class_init (GeglDataClass * klass);
static void init (GeglData *self, GeglDataClass * klass);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

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

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_DATA_NAME,
                                   g_param_spec_string ("data_name",
                                                        "DataName",
                                                        "The GeglData's name",
                                                        "", 
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

}

static void 
init (GeglData * self, 
      GeglDataClass * klass)
{
  self->value = g_new0(GValue, 1); 
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

  g_free(self->name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglData * data = GEGL_DATA(gobject);
  switch (prop_id)
  {
    case PROP_DATA_NAME:
      gegl_data_set_name(data, g_value_get_string(value));  
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
  GeglData * data = GEGL_DATA(gobject);
  switch (prop_id)
  {
    case PROP_DATA_NAME:
      g_value_set_string(value, gegl_data_get_name(data));  
      break;
    default:
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

/**
 * gegl_data_copy_value:
 * @self: a #GeglData
 * @value: the value to copy.
 *
 * Copy the value for this data.  
 *
 **/
void
gegl_data_copy_value (GeglData * self, 
                      const GValue *value)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_DATA (self));
  g_return_if_fail (G_IS_VALUE(value));

  g_value_unset(self->value);
  g_value_init(self->value, value->g_type);
  g_value_copy(value, self->value);
}

/**
 * gegl_data_set_name:
 * @self: a #GeglData.
 * @name: a string 
 *
 * Sets the name for this data.
 *
 **/
void 
gegl_data_set_name (GeglData * self, 
                    const gchar * name)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_DATA (self));

  self->name = g_strdup(name);
}

/**
 * gegl_data_get_name:
 * @self: a #GeglData.
 *
 * Gets the name for this data.
 *
 * Returns: a string for the name of this data.
 **/
G_CONST_RETURN gchar*
gegl_data_get_name (GeglData * self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GEGL_IS_DATA (self), NULL);

  return self->name;
}
