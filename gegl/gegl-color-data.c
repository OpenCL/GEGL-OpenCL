#include "gegl-color-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglColorDataClass * klass);
static void init (GeglColorData *self, GeglColorDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_color_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglColorData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_DATA, 
                                     "GeglColorData", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }

    return type;
}

static void 
class_init (GeglColorDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglColorData * self, 
      GeglColorDataClass * klass)
{
  self->color_model = NULL;
}

GeglColorModel * 
gegl_color_data_get_color_model (GeglColorData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_DATA (self), NULL);
   
  return self->color_model;
}

void
gegl_color_data_set_color_model (GeglColorData * self,
                                  GeglColorModel *color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR_DATA (self));
   
  self->color_model = color_model;
}
