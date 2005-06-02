#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-mock-object.h"


static void class_init        (GeglMockObjectClass * klass);
static gpointer parent_class = NULL;

GType
gegl_mock_object_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockObjectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockObject),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT,
                                     "GeglMockObject",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GeglMockObjectClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}
