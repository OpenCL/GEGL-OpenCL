#include "gegl-op.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-visitor.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-eval-mgr.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglOpClass * klass);
static void init (GeglOp * self, GeglOpClass * klass);
static void finalize(GObject * gobject);
static void notify_num_outputs(GObject * self, GParamSpec *pspec);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void free_attributes_array(GeglOp * self);
static void init_attributes(GeglOp *self);

static void accept (GeglNode * node, GeglVisitor * visitor);

static gpointer parent_class = NULL;

GType
gegl_op_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglOpClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglOp),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_NODE , 
                                     "GeglOp", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;
  klass->init_attributes = init_attributes;

  return;
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  self->attributes = NULL;
  g_object_connect(self, 
                   "signal::notify::num-outputs", 
                   notify_num_outputs, 
                   NULL, 
                   NULL); 
  return;
}

static void
notify_num_outputs(GObject * self, 
                   GParamSpec *pspec)
{
  GeglOp *op = GEGL_OP(self);
  gegl_op_init_attributes(op);
}

static void
finalize(GObject *gobject)
{
  GeglOp *self = GEGL_OP(gobject);

  free_attributes_array(self);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
}


/**
 * gegl_op_apply
 * @self: a #GeglOp.
 *
 * Apply the op. 
 *
 **/
void      
gegl_op_apply(GeglOp * self)
{
  gegl_op_apply_roi(self, NULL);
}

/**
 * gegl_op_apply_roi
 * @self: a #GeglOp.
 * @roi: a #GeglRect.
 *
 * Apply the op for the roi. 
 *
 **/
void      
gegl_op_apply_roi(GeglOp * self, 
                  GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  {
    GeglEvalMgr * eval_mgr = g_object_new(GEGL_TYPE_EVAL_MGR,
                                          "root", self,
                                          "roi", roi,
                                          NULL);
    gegl_eval_mgr_evaluate(eval_mgr);
    g_object_unref(eval_mgr);
  }
}

/**
 * gegl_op_init_attributes:
 * @self: a #GeglOp.
 *
 * Allocates the attributes array for this op. 
 *
 **/
void 
gegl_op_init_attributes(GeglOp * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  {
    GeglOpClass *klass = GEGL_OP_GET_CLASS(self);

    if(klass->init_attributes)
      (*klass->init_attributes)(self);
  }
}


static void
free_attributes_array (GeglOp *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP(self));

  if(self->attributes)
    {
      g_free(self->attributes);
      self->attributes = NULL;
    }
}

static void
init_attributes(GeglOp *self)
{
  gint num_output_values = gegl_node_get_num_outputs(GEGL_NODE(self));

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP(self));
  g_return_if_fail (num_output_values >= 0);

  /* Free the old array of pointers. */
  free_attributes_array(self);

  /* Just allocate the array, leave subclasses to fill in */
  if(num_output_values > 0)
    self->attributes = g_new0(GeglAttributes*, num_output_values); 
}

/**
 * gegl_op_get_attributes:
 * @self: a #GeglOp.
 *
 * Get the attributes for this op, as a list. 
 *
 * Returns: a GList of attributes.
 **/
GList *
gegl_op_get_attributes(GeglOp * self) 
{
  GList * attributes = NULL;
  gint i;
  gint num_outputs = GEGL_NODE(self)->num_outputs;

  for(i = 0; i < num_outputs; i++)
    attributes = g_list_append(attributes, self->attributes[i]); 

  return attributes;
}

/**
 * gegl_op_get_nth_attributes:
 * @self: a #GeglOp.
 * @n: which attributes we want.
 *
 * Get attributes for the nth output of this Op. 
 *
 * Returns: a pointer to the Attributes.
 **/
GeglAttributes *
gegl_op_get_nth_attributes(GeglOp *self,
                           gint n) 
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);
  g_return_val_if_fail (n >=0, NULL);

  if(n == 0 && GEGL_NODE(self)->num_outputs == 0)
    return NULL;
  else
    g_return_val_if_fail (n < GEGL_NODE(self)->num_outputs, NULL);

  return self->attributes[n];
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_op(visitor, GEGL_OP(node));
}
