#include "gegl-op-impl.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglOpImplClass * klass);
static void init (GeglOpImpl * self, GeglOpImplClass * klass);

static gpointer parent_class = NULL;

GType
gegl_op_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglOpImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglOpImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , "GeglOpImpl", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglOpImplClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);

  klass->prepare = NULL;
  klass->process = NULL;
  klass->finish = NULL;

  return;
}

static void 
init (GeglOpImpl * self, 
      GeglOpImplClass * klass)
{
  self->num_inputs = 0;
  return;
}

gint 
gegl_op_impl_get_num_inputs (GeglOpImpl * self)
{
  g_return_val_if_fail (self != NULL, -1);
  g_return_val_if_fail (GEGL_IS_OP_IMPL (self), -1);
  return self->num_inputs;
}

/**
 * gegl_op_impl_prepare:
 * @self: a #GeglOpImpl.
 * @request_list: #GeglOpImplRequest list.
 *
 *  Prepares to do the op_impl. Subclasses set up for the op_impl here.
 *  Inputs are completely determined when this is called.  
 *
 **/
void 
gegl_op_impl_prepare (GeglOpImpl * self, 
                     GList * request_list)
{
  GeglOpImplClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP_IMPL (self));
  klass = GEGL_OP_IMPL_GET_CLASS(self);

  if(klass->prepare)
    (*klass->prepare)(self,request_list);
}

/**
 * gegl_op_impl_process:
 * @self: a #GeglOpImpl.
 * @request_list: #GeglOpImplRequest list.
 *
 *  Does the op_impl.
 *
 **/
void 
gegl_op_impl_process (GeglOpImpl * self, 
                     GList * request_list)
{
  GeglOpImplClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP_IMPL (self));
  klass = GEGL_OP_IMPL_GET_CLASS(self);

  if(klass->process)
    (*klass->process)(self,request_list);
}

/**
 * gegl_op_impl_finish:
 * @self: a #GeglOpImpl.
 * @request_list: #GeglOpImplRequest list.
 *
 * Cleans up after the op_impl.
 *
 **/
void 
gegl_op_impl_finish (GeglOpImpl * self, 
                    GList * request_list)
{
  GeglOpImplClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OP_IMPL (self));
  klass = GEGL_OP_IMPL_GET_CLASS(self);

  if(klass->finish)
    (*klass->finish)(self,request_list);
}
