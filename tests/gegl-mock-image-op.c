#include "gegl-mock-image-op.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglMockImageOpClass * klass);
static void init(GeglMockImageOp *self, GeglMockImageOpClass *klass);
static gpointer parent_class = NULL;

GType
gegl_mock_image_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockImageOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockImageOp),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE_OP, 
                                     "GeglMockImageOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockImageOpClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
  return;
}

static void 
init (GeglMockImageOp * self, 
      GeglMockImageOpClass * klass)
{
}
