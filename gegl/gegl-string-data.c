#include "gegl-string-data.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglStringDataClass * klass);
static void init (GeglStringData *self, GeglStringDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_string_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglStringDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglStringData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_DATA, 
                                     "GeglStringData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglStringDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglStringData * self, 
      GeglStringDataClass * klass)
{
  GeglData * data = GEGL_DATA(self);
  g_value_init(data->value, G_TYPE_STRING);
  g_value_set_string(data->value, "");
}
