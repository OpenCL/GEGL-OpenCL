#include "gegl-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglPointOpClass * klass);
static void init (GeglPointOp * self, GeglPointOpClass * klass);
static void finalize (GObject * gobject);

static void process (GeglOp * self, GList *output_values, GList * input_values);

static gpointer parent_class = NULL;

GType
gegl_point_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPointOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPointOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE , 
                                     "GeglPointOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglPointOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);
  gobject_class->finalize = finalize;

  op_class->process = process;

  return;
}

static void 
init (GeglPointOp * self, 
      GeglPointOpClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op = GEGL_OP(self);
  return;
}

static void 
finalize (GObject * gobject)
{
  GeglPointOp * self_point_op = GEGL_POINT_OP(gobject);

  g_object_unref(self_point_op->scanline_processor);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void 
process (GeglOp * self_op, 
         GList * output_values,
         GList * input_values)
{
  GeglPointOp *self =  GEGL_POINT_OP(self_op);
  /*
  LOG_DEBUG("process", "calling scanline processor"); 
  */
  gegl_scanline_processor_process(self->scanline_processor, 
                                  output_values,
                                  input_values);
}
