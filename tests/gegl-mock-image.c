#include "gegl-mock-image.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMockImageClass * klass);
static void init(GeglMockImage *self, GeglMockImageClass *klass);
static gpointer parent_class = NULL;

GType
gegl_mock_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockImage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE, 
                                     "GeglMockImage", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockImageClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
  return;
}

static void 
init (GeglMockImage * self, 
      GeglMockImageClass * klass)
{
  g_object_set(self, "num_inputs", 0, NULL);
  g_object_set(self, "num_outputs", 1, NULL);
}
