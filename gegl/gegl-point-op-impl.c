#include "gegl-point-op-impl.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglPointOpImplClass * klass);
static void init (GeglPointOpImpl * self, GeglPointOpImplClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize (GObject * gobject);

static void process (GeglOpImpl * self_op_impl, GList * requests);

static gpointer parent_class = NULL;

GType
gegl_point_op_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPointOpImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPointOpImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE_IMPL, 
                                     "GeglPointOpImpl", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPointOpImplClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpImplClass *op_impl_class = GEGL_OP_IMPL_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;

  op_impl_class->process = process;

  return;
}

static void 
init (GeglPointOpImpl * self, 
      GeglPointOpImplClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op_impl = GEGL_OP_IMPL(self);
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglPointOpImpl *self = GEGL_POINT_OP_IMPL(gobject);
  GeglOpImpl *self_op_impl = GEGL_OP_IMPL(gobject);
  gint i;
  gint num_inputs = gegl_op_impl_get_num_inputs(self_op_impl);
  self->input_offsets = g_new (GeglPoint, num_inputs);

  for (i = 0; i < num_inputs; i++)
  {
    self->input_offsets[i].x = 0;
    self->input_offsets[i].y = 0;
  }     

  return gobject;
}

static void 
finalize (GObject * gobject)
{
  GeglPointOpImpl * self_point_op_impl = GEGL_POINT_OP_IMPL(gobject);

  g_free(self_point_op_impl->input_offsets); 
  g_object_unref(self_point_op_impl->scanline_processor);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

void             
gegl_point_op_impl_get_nth_input_offset  (GeglPointOpImpl * self, 
                                         gint i,
                                         GeglPoint *point)
{
  g_return_if_fail(self);
  g_return_if_fail(point);

  {
    GeglOpImpl * self_op_impl = GEGL_OP_IMPL(self);

    if (i >= self_op_impl->num_inputs || i < 0)
      g_warning("Cant get offset of %d th input", i);

    point->x = self->input_offsets[i].x;
    point->y = self->input_offsets[i].y;
  }
}

void             
gegl_point_op_impl_set_nth_input_offset  (GeglPointOpImpl * self, 
                                         gint i,
                                         GeglPoint * point)
{
  g_return_if_fail(self);
  g_return_if_fail(point);

  {
    GeglOpImpl * self_op_impl = GEGL_OP_IMPL(self);

    if (i >= self_op_impl->num_inputs || i < 0)
      g_warning("Cant set offset of %d th input", i);

    self->input_offsets[i].x = point->x;
    self->input_offsets[i].y = point->y;
  }
}

static void 
process (GeglOpImpl * self_op_impl, 
         GList * requests)
{
  GeglPointOpImpl *self =  GEGL_POINT_OP_IMPL(self_op_impl);
  gegl_scanline_processor_process(self->scanline_processor, requests);
}
