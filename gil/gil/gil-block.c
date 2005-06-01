#include "gil-block.h"

static void class_init (GilBlockClass * klass);
static void init(GilBlock *self, GilBlockClass *klass);
static void finalize(GObject * self_object);

static gpointer parent_class = NULL;

GType
gil_block_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GilBlockClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GilBlock),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GIL_TYPE_STATEMENT,
                                     "GilBlock",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GilBlockClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  return;
}

static void
init (GilBlock * self,
      GilBlockClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}
