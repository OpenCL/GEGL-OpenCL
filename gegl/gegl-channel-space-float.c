#include "gegl-channel-space-float.h"
#include "gegl-object.h"
#include "gegl-channel-value-types.h"
#include "gegl-value-types.h"

static void class_init (GeglChannelSpaceFloatClass * klass);
static void init (GeglChannelSpaceFloat * self, GeglChannelSpaceFloatClass * klass);
static void convert_to_float (GeglChannelSpace * channel_space, gfloat * dest, gpointer src, gint num);
static void convert_from_float (GeglChannelSpace * channel_space, gpointer dest, gfloat *src, gint num);
static void convert_value_to_float (GeglChannelSpace * channel_space, GValue *dest, GValue*src);
static void convert_value_from_float (GeglChannelSpace * channel_space, GValue *dest, GValue*src);

static gpointer parent_class = NULL;

GType
gegl_channel_space_float_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglChannelSpaceFloatClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglChannelSpaceFloat),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_CHANNEL_SPACE, 
                                     "GeglChannelSpaceFloat", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglChannelSpaceFloatClass * klass)
{
  GeglChannelSpaceClass *channel_space_class = GEGL_CHANNEL_SPACE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  channel_space_class->convert_to_float = convert_to_float;
  channel_space_class->convert_from_float = convert_from_float; 
  channel_space_class->convert_value_to_float = convert_value_to_float;
  channel_space_class->convert_value_from_float = convert_value_from_float; 
}

static void 
init (GeglChannelSpaceFloat * self, 
      GeglChannelSpaceFloatClass * klass)
{
  GeglChannelSpace *channel_space = GEGL_CHANNEL_SPACE(self);
  channel_space->channel_space_type = GEGL_CHANNEL_SPACE_FLOAT;
  channel_space->name = g_strdup("float");
  channel_space->is_channel_data = TRUE;
  channel_space->bits = 32;
}

static void 
convert_value_to_float (GeglChannelSpace * channel_space, 
                        GValue *dest,
                        GValue *src)
{
  gfloat s = g_value_get_channel_float(src);
  g_value_set_channel_float(dest, s);
}

static void 
convert_to_float (GeglChannelSpace * channel_space, 
                  gfloat * dest, 
                  gpointer src, 
                  gint num)
{
  gfloat *s = (gfloat*)src;
  while(num--)
    *dest++ = *s++;
}

static void 
convert_value_from_float (GeglChannelSpace * channel_space, 
                          GValue *dest,
                          GValue *src)
{
  gfloat s = g_value_get_channel_float(src);
  g_value_set_channel_float(dest, s);
}

static void 
convert_from_float (GeglChannelSpace * channel_space, 
                    gpointer dest, 
                    gfloat *src, 
                    gint num)
{
  gfloat *d = (gfloat*)dest;
  while(num--)
    *d++ = *src++;
}
