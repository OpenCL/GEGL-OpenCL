#include "gegl-sampled-image-impl.h"
#include "gegl-image-impl.h"
#include "gegl-image-mgr.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-utils.h"

static void class_init (GeglSampledImageImplClass * klass);
static void init(GeglSampledImageImpl *self, GeglSampledImageImplClass *klass);

static gpointer parent_class = NULL;

GType
gegl_sampled_image_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglSampledImageImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglSampledImageImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE_IMPL, "GeglSampledImageImpl", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglSampledImageImplClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
   
  return;
}

static void 
init (GeglSampledImageImpl * self, 
      GeglSampledImageImplClass * klass)
{
  GeglOpImpl *self_op_impl = GEGL_OP_IMPL(self);
  self_op_impl->num_inputs = 0;
  return;
}

gint 
gegl_sampled_image_impl_get_width (GeglSampledImageImpl * self)
{
  return self->width;
}

void
gegl_sampled_image_impl_set_width (GeglSampledImageImpl * self, 
                                    gint width)
{
  self->width = width;
}

gint 
gegl_sampled_image_impl_get_height (GeglSampledImageImpl * self)
{
  return self->height;
}

void
gegl_sampled_image_impl_set_height (GeglSampledImageImpl * self, 
                                     gint height)
{
  self->height = height;
}
