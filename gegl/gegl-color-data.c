#include "gegl-color-data.h"
#include "gegl-color.h"
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
                                     0);
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
  GeglData * data = GEGL_DATA(self);
  g_value_init(data->value, GEGL_TYPE_COLOR);

  self->color_space = NULL;
}

GeglColorSpace * 
gegl_color_data_get_color_space (GeglColorData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_DATA (self), NULL);
   
  return self->color_space;
}

void
gegl_color_data_set_color_space (GeglColorData * self,
                                 GeglColorSpace *color_space)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COLOR_DATA (self));
   
  self->color_space = color_space;
}

gfloat *
gegl_color_data_get_components(GeglColorData *self,
                               gint *num_components)
{
  GeglData *data = GEGL_DATA(self);
  GeglColor *color = g_value_get_object(data->value);
  gfloat *pixel = NULL;
  g_object_get(color, "components", num_components, &pixel, NULL); 

  return pixel;
}
