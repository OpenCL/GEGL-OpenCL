#include "gegl-stat-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglStatOpClass * klass);
static void init (GeglStatOp * self, GeglStatOpClass * klass);
static void finalize(GObject * gobject);

static void process (GeglOp * self, GList *output_values, GList * input_values);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  op_class->process = process;

  return;
}

static void 
init (GeglStatOp * self, 
      GeglStatOpClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op = GEGL_OP(self);
  return;
}

static void
finalize(GObject *gobject)
{
  GeglStatOp * self_stat_op = GEGL_STAT_OP(gobject);
  g_object_unref(self_stat_op->scanline_processor);
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
process (GeglOp * self_op, 
         GList * output_values,
         GList * input_values)
{
  GeglStatOp *self =  GEGL_STAT_OP(self_op);
  /*
  LOG_DEBUG("process", "calling scanline processor"); 
  */
  gegl_scanline_processor_process(self->scanline_processor, 
                                  output_values,
                                  input_values);
}
