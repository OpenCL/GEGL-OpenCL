#include "gegl-channel-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglChannelDataClass * klass);
static void init (GeglChannelData *self, GeglChannelDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_channel_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglChannelDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglChannelData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_DATA, 
                                     "GeglChannelData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglChannelDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglChannelData * self, 
      GeglChannelDataClass * klass)
{
  self->data_space = NULL;
}

GeglDataSpace * 
gegl_channel_data_get_data_space (GeglChannelData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_CHANNEL_DATA (self), NULL);
   
  return self->data_space;
}
