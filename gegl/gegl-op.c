#include "gegl-op.h"
#include "gegl-node.h"
#include "gegl-object.h"
#include "gegl-sampled-image.h"
#include "gegl-image-mgr.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_SOURCE0,
  PROP_SOURCE1,
  PROP_LAST 
};

static void class_init (GeglOpClass * klass);
static void init (GeglOp * self, GeglOpClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

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
                                     0);
    }
    return type;
}

static void 
class_init (GeglOpClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  g_object_class_install_property (gobject_class, PROP_SOURCE0,
                                   g_param_spec_object ("source0",
                                                        "Source0",
                                                        "The GeglOp source0",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SOURCE1,
                                   g_param_spec_object ("source1",
                                                        "Source1",
                                                        "The GeglOp source1",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglOp * self, 
      GeglOpClass * klass)
{
  self->alt_input = 0;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglOp *self = GEGL_OP (gobject);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglOp *self = GEGL_OP (gobject);

  switch (prop_id)
  {
    case PROP_SOURCE0:
      {
        GeglOp *source0 = (GeglOp*)g_value_get_object(value);
        if(GEGL_OBJECT(self)->constructed || source0)
          gegl_op_set_source0(self, source0);  
      }
      break;
    case PROP_SOURCE1:
      {
        GeglOp *source1 = (GeglOp*)g_value_get_object(value);
        if(GEGL_OBJECT(self)->constructed || source1)
          gegl_op_set_source1(self, source1);  
      }
      break;
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
  GeglOp *self = GEGL_OP (gobject);

  switch (prop_id)
  {
    case PROP_SOURCE0:
      g_value_set_object (value, (GObject*)(gegl_op_get_source0 (self)));
      break;
    case PROP_SOURCE1:
      g_value_set_object (value, (GObject*)(gegl_op_get_source1 (self)));
      break;
    default:
      break;
  }
}

/**
 * gegl_op_prepare:
 * @self: a #GeglOp.
 * @request_list: #GeglOpRequest list.
 *
 *  Prepares to do the op. Subclasses set up for the op here.
 *  Inputs are completely determined when this is called.  
 *
 **/
void 
gegl_op_prepare (GeglOp * self, 
                     GList * request_list)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->prepare)
    (*klass->prepare)(self,request_list);
}

/**
 * gegl_op_process:
 * @self: a #GeglOp.
 * @request_list: #GeglOpRequest list.
 *
 *  Does the op.
 *
 **/
void 
gegl_op_process (GeglOp * self, 
                     GList * request_list)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->process)
    (*klass->process)(self,request_list);
}

/**
 * gegl_op_finish:
 * @self: a #GeglOp.
 * @request_list: #GeglOpRequest list.
 *
 * Cleans up after the op.
 *
 **/
void 
gegl_op_finish (GeglOp * self, 
                    GList * request_list)
{
  GeglOpClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  klass = GEGL_OP_GET_CLASS(self);

  if(klass->finish)
    (*klass->finish)(self,request_list);
}

/**
 * gegl_op_get_alt_input:
 * @self: a #GeglOp.
 *
 * Gets the index of input to use instead of this op.  Allows traversals to
 * pass through this op if not enabled or if looking for a particular kind of
 * op. Image data can pass through non-Image ops this way. 
 *
 * Returns: the input's index to use instead of this op. 
 **/
gint 
gegl_op_get_alt_input (GeglOp * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_OP (self), (gint )0);
   
  return self->alt_input;
}

/**
 * gegl_op_apply:
 * @self: a #GeglOp.
 * @dest: a #GeglSampledImage dest image.
 * @roi: a #GeglRect region of interest.
 *
 * This evaluates the graph rooted on this op.
 *
 **/
void 
gegl_op_apply (GeglOp * self, 
               GeglSampledImage * dest, 
               GeglRect * roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  {
    /* Get the singleton image manager. */
    GeglImageMgr *image_mgr = gegl_image_mgr_instance();

    /* Call image manager, which does the operation. */
    gegl_image_mgr_apply(image_mgr,self,dest,roi);
    g_object_unref(image_mgr);
  }
}

GeglOp*
gegl_op_get_source0 (GeglOp * self) 
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);
  return (GeglOp*)gegl_node_get_nth_input(GEGL_NODE(self), 0);
}

void
gegl_op_set_source0 (GeglOp * self, 
                     GeglOp *source) 
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));
  gegl_node_set_nth_input(GEGL_NODE(self), (GeglNode*)source, 0);
}

GeglOp*
gegl_op_get_source1 (GeglOp * self) 
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_OP (self), NULL);
  return (GeglOp*)(gegl_node_get_nth_input(GEGL_NODE(self), 1));
}

void
gegl_op_set_source1 (GeglOp * self, 
                     GeglOp *source) 
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP (self));

  gegl_node_set_nth_input(GEGL_NODE(self), (GeglNode*)source, 1);
}
