#include "gegl-stat-op.h"
#include "gegl-stat-op-impl.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglStatOpClass * klass);
static void init (GeglStatOp * self, GeglStatOpClass * klass);
static void finalize(GObject * gobject);

static gpointer parent_class = NULL;

GType
gegl_stat_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglStatOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglStatOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglStatOp", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglStatOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  return;
}

static void 
init (GeglStatOp * self, 
      GeglStatOpClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}
