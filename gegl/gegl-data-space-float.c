#include "gegl-data-space-float.h"
#include "gegl-object.h"
#include "gegl-value-types.h"

static void class_init (GeglDataSpaceFloatClass * klass);
static void init (GeglDataSpaceFloat * self, GeglDataSpaceFloatClass * klass);
static void convert_to_float (GeglDataSpace * data_space, gfloat * dest, gpointer src, gint num);
static void convert_from_float (GeglDataSpace * data_space, gpointer dest, gfloat *src, gint num);
static void convert_value_to_float (GeglDataSpace * data_space, GValue *dest, GValue*src);
static void convert_value_from_float (GeglDataSpace * data_space, GValue *dest, GValue*src);

static gpointer parent_class = NULL;

GType
gegl_data_space_float_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDataSpaceFloatClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDataSpaceFloat),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DATA_SPACE, 
                                     "GeglDataSpaceFloat", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglDataSpaceFloatClass * klass)
{
  GeglDataSpaceClass *data_space_class = GEGL_DATA_SPACE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  data_space_class->convert_to_float = convert_to_float;
  data_space_class->convert_from_float = convert_from_float; 
  data_space_class->convert_value_to_float = convert_value_to_float;
  data_space_class->convert_value_from_float = convert_value_from_float; 
}

static void 
init (GeglDataSpaceFloat * self, 
      GeglDataSpaceFloatClass * klass)
{
  GeglDataSpace *data_space = GEGL_DATA_SPACE(self);
  data_space->data_space_type = GEGL_DATA_SPACE_FLOAT;
  data_space->name = g_strdup("float");
  data_space->is_channel_data = TRUE;
  data_space->bits = 32;
}

static void 
convert_value_to_float (GeglDataSpace * data_space, 
                        GValue *dest,
                        GValue *src)
{
  gfloat s = g_value_get_gegl_float(src);
  g_value_set_gegl_float(dest, s);
}

static void 
convert_to_float (GeglDataSpace * data_space, 
                  gfloat * dest, 
                  gpointer src, 
                  gint num)
{
  gfloat *s = (gfloat*)src;
  while(num--)
    *dest++ = *s++;
}

static void 
convert_value_from_float (GeglDataSpace * data_space, 
                          GValue *dest,
                          GValue *src)
{
  gfloat s = g_value_get_gegl_float(src);
  g_value_set_gegl_float(dest, s);
}

static void 
convert_from_float (GeglDataSpace * data_space, 
                    gpointer dest, 
                    gfloat *src, 
                    gint num)
{
  gfloat *d = (gfloat*)dest;
  while(num--)
    *d++ = *src++;
}
