#include "gegl-pixel-data.h"
#include "gegl-pixel-value-types.h"
#include "gegl-pixel-param-specs.h"
#include "gegl-utils.h"

static void class_init (GeglPixelDataClass * klass);
static void init (GeglPixelData *self, GeglPixelDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_pixel_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPixelDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglPixelData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_DATA, 
                                     "GeglPixelData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglPixelDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglPixelData * self, 
      GeglPixelDataClass * klass)
{
  GeglData * data = GEGL_DATA(self);
  g_value_init(data->value, GEGL_TYPE_PIXEL_RGB_FLOAT);
  GEGL_COLOR_DATA(self)->color_model = gegl_color_model_instance("rgb-float");
}
