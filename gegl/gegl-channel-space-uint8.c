#include "gegl-channel-space-uint8.h"
#include "gegl-object.h"
#include "gegl-value-types.h"

static void class_init (GeglChannelSpaceU8Class * klass);
static void init (GeglChannelSpaceU8 * self, GeglChannelSpaceU8Class * klass);
static void convert_to_float (GeglChannelSpace * channel_space, gfloat * dest, gpointer src, gint num);
static void convert_from_float (GeglChannelSpace * channel_space, gpointer dest, gfloat *src, gint num);
static void convert_value_to_float (GeglChannelSpace * channel_space, GValue *dest, GValue*src);
static void convert_value_from_float (GeglChannelSpace * channel_space, GValue *dest, GValue*src);

static gpointer parent_class = NULL;

GType
gegl_channel_space_uint8_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglChannelSpaceU8Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglChannelSpaceU8),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_CHANNEL_SPACE, 
                                     "GeglChannelSpaceU8", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglChannelSpaceU8Class * klass)
{
  GeglChannelSpaceClass *channel_space_class = GEGL_CHANNEL_SPACE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  channel_space_class->convert_to_float = convert_to_float;
  channel_space_class->convert_from_float = convert_from_float; 
  channel_space_class->convert_value_to_float = convert_value_to_float;
  channel_space_class->convert_value_from_float = convert_value_from_float; 
}

static void 
init (GeglChannelSpaceU8 * self, 
      GeglChannelSpaceU8Class * klass)
{
  GeglChannelSpace *channel_space = GEGL_CHANNEL_SPACE(self);
  channel_space->channel_space_type = GEGL_CHANNEL_SPACE_UINT8;
  channel_space->name = g_strdup("uint8");
  channel_space->is_channel_data = TRUE;
  channel_space->bits = 8;
}

static void 
convert_to_float (GeglChannelSpace * channel_space, 
                  gfloat * dest, 
                  gpointer src, 
                  gint num)
{
  guint8 *s = (guint8*)src;
  gfloat *d = dest;
  while(num--)
    {
      *d++ = *s++/255.0;
    }
}

static void 
convert_from_float (GeglChannelSpace * channel_space, 
                    gpointer dest, 
                    gfloat *src, 
                    gint num)
{
  guint8 *d = (guint8*)dest;
  gfloat *s = src;
  while(num--)
    {
      *d++ = CLAMP((gint)(255 * *s + .5), 0, 255);
      s++;
    }
}

static void 
convert_value_to_float (GeglChannelSpace * channel_space, 
                        GValue *dest,
                        GValue *src)
{
  gint s = g_value_get_channel_uint8(src);
  gfloat d = s / 255.0;
  g_value_set_channel_float(dest, d);
}


static void 
convert_value_from_float (GeglChannelSpace * channel_space, 
                          GValue *dest,
                          GValue *src)
{
  gfloat s = g_value_get_channel_float(src);
  guint8 d = CLAMP((gint)(255 * s + .5), 0, 255);
  g_value_set_channel_uint8(dest, d);
}
