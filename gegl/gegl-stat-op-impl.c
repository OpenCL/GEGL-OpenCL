#include "gegl-stat-op-impl.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglStatOpImplClass * klass);
static void init (GeglStatOpImpl * self, GeglStatOpImplClass * klass);
static void finalize (GObject * gobject);

static void process (GeglOpImpl * self_op_impl, GList * requests);

static gpointer parent_class = NULL;

GType
gegl_stat_op_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglStatOpImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglStatOpImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP_IMPL, 
                                     "GeglStatOpImpl", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglStatOpImplClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpImplClass *op_impl_class = GEGL_OP_IMPL_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  op_impl_class->process = process;

  return;
}

static void 
init (GeglStatOpImpl * self, 
      GeglStatOpImplClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op_impl = GEGL_OP_IMPL(self);
  return;
}

static void 
finalize (GObject * gobject)
{
  GeglStatOpImpl * self_stat_op_impl = GEGL_STAT_OP_IMPL(gobject);

  g_object_unref(self_stat_op_impl->scanline_processor);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void 
process (GeglOpImpl * self_impl, 
         GList * requests)
{
  GeglStatOpImpl *self =  GEGL_STAT_OP_IMPL(self_impl);
  gegl_scanline_processor_process(self->scanline_processor, requests);
}
