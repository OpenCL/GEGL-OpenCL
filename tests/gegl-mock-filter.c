#include "gegl-mock-filter.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init        (GeglMockFilterClass * klass);
static void init (GeglMockFilter * self, GeglMockFilterClass * klass);
static void finalize (GObject * gobject);
static gpointer parent_class = NULL;

GType
gegl_mock_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_FILTER, 
                                     "GeglMockFilter", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglMockFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
}

static void 
init (GeglMockFilter * self, 
      GeglMockFilterClass * klass)
{
  g_object_set(self, "num_outputs", 0, NULL);
  g_object_set(self, "num_inputs", 0, NULL);
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}
