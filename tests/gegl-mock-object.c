#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-mock-object.h"


static void gegl_mock_object_class_init (GeglMockObjectClass *klass);
static void gegl_mock_object_init       (GeglMockObject      *self);


G_DEFINE_TYPE (GeglMockObject, gegl_mock_object, GEGL_TYPE_OBJECT)


static void
gegl_mock_object_class_init (GeglMockObjectClass *klass)
{
}

static void
gegl_mock_object_init (GeglMockObject *self)
{
}
