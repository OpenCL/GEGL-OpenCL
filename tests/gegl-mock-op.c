#include "gegl-mock-op.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init        (GeglMockOpClass * klass);
static void init (GeglMockOp * self, GeglMockOpClass * klass);
static void finalize (GObject * gobject);
static gpointer parent_class = NULL;

GType
gegl_mock_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP, 
                                     "GeglMockOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
}

static void 
init (GeglMockOp * self, 
      GeglMockOpClass * klass)
{
  g_object_set(self, "num_outputs", 0, NULL);
  g_object_set(self, "num_inputs", 0, NULL);
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}
