#include "gegl-point-op.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglPointOpClass * klass);
static void init (GeglPointOp * self, GeglPointOpClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize (GObject * gobject);

static void compute_preimage (GeglImage * self_op, GeglRect * preimage, GeglRect * rect, gint i);
static void compute_have_rect (GeglImage * self_op, GeglRect *have_rect, GList * input_have_rects);
static void compute_result_rect (GeglImage * self_op, GeglRect * result_rect, GeglRect * need_rect, GeglRect *have_rect);

static void process (GeglOp * self, GList * request_list);

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
                                     0);
    }
    return type;
}

static void 
class_init (GeglPointOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);
  GeglImageClass *image_class = GEGL_IMAGE_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;

  op_class->process = process;

  image_class->compute_preimage = compute_preimage;
  image_class->compute_have_rect = compute_have_rect;
  image_class->compute_result_rect = compute_result_rect;

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

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglPointOp *self = GEGL_POINT_OP(gobject);
  GeglNode *self_node = GEGL_NODE(gobject);
  gint i;
  gint num_inputs = gegl_node_num_inputs(self_node);
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
  GeglPointOp * self_point_op = GEGL_POINT_OP(gobject);

  g_free(self_point_op->input_offsets); 
  g_object_unref(self_point_op->scanline_processor);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

void             
gegl_point_op_get_nth_input_offset  (GeglPointOp * self, 
                                     gint i,
                                     GeglPoint *point)
{
  g_return_if_fail(self);
  g_return_if_fail(point);

  {
    GeglNode * self_node = GEGL_NODE(self);

    if (i >= gegl_node_num_inputs(self_node) || i < 0)
      g_warning("Cant get offset of %d th input", i);

    point->x = self->input_offsets[i].x;
    point->y = self->input_offsets[i].y;
  }
}

void             
gegl_point_op_set_nth_input_offset  (GeglPointOp * self, 
                                     gint i,
                                     GeglPoint * point)
{
  g_return_if_fail(self);
  g_return_if_fail(point);

  {
    GeglNode * self_node = GEGL_NODE(self);

    if (i >= gegl_node_num_inputs(self_node) || i < 0)
      g_warning("Cant get offset of %d th input", i);

    self->input_offsets[i].x = point->x;
    self->input_offsets[i].y = point->y;
  }
}

static void 
process (GeglOp * self_op, 
         GList * requests)
{
  GeglPointOp *self =  GEGL_POINT_OP(self_op);
  gegl_scanline_processor_process(self->scanline_processor, requests);
}

static void 
compute_have_rect (GeglImage * self_image, 
                   GeglRect * have_rect,
                   GList * inputs_have_rects)
{
  GeglPointOp *self = GEGL_POINT_OP(self_image);
  gint num_inputs = g_list_length(inputs_have_rects);
  GeglRect intersection;
  GeglRect input_have_rect;
  gint i;

  /* Start off as big as possible. */
  gegl_rect_set(have_rect, 0, 0, G_MAXINT, G_MAXINT);

  /* Take the intersection of all the inputs have_rects. */
  for (i = 0; i < num_inputs; i++)
    {
      GeglPoint input_offset;

      /* Get the have_rect of ith input, in its coords */
      gegl_rect_copy(&input_have_rect, (GeglRect*)g_list_nth_data(inputs_have_rects,i));
      gegl_point_op_get_nth_input_offset(self, i, &input_offset); 

      /* Put in the offsets for this op */
      input_have_rect.x += input_offset.x;
      input_have_rect.y += input_offset.y;

      /* Take intersection of input_have_rect and have_rect, store in have_rect */
      gegl_rect_intersect (&intersection, &input_have_rect, have_rect); 
      gegl_rect_copy(have_rect, &intersection);
    }
}

static void 
compute_preimage (GeglImage * self_image, 
                  GeglRect * preimage, 
                  GeglRect * rect, 
                  gint i)
{
  GeglPointOp *self = GEGL_POINT_OP(self_image); 
  GeglPoint input_offset;

  gegl_rect_copy (preimage, rect);
  gegl_point_op_get_nth_input_offset(self, i, &input_offset); 


  /* 
     Just take out the offset of this input,
     so put preimage in the inputs coord system.
  */ 
  preimage->x -= input_offset.x;
  preimage->y -= input_offset.y;
}

static void 
compute_result_rect (GeglImage * self_image, 
                     GeglRect * result_rect, 
                     GeglRect * need_rect, 
                     GeglRect * have_rect)
{
  /* By default just intersect result and need */
  gegl_rect_intersect (result_rect, need_rect, have_rect);
}
