#include "gegl-image-impl.h"
#include "gegl-op-impl.h"
#include "gegl-image-mgr.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"

static void init (GeglImageImpl * self, GeglImageImplClass * klass);
static void class_init (GeglImageImplClass * klass);
static void finalize(GObject * self_object);

static gpointer parent_class = NULL;

GType
gegl_image_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP_IMPL, "GeglImageImpl", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglImageImplClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  return;
}

static void 
init (GeglImageImpl * self, 
      GeglImageImplClass * klass)
{
  self->color_model = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglImageImpl *self = GEGL_IMAGE_IMPL (gobject);

  LOG_DEBUG("finalize", "deleting %x", (gint)(gobject));

  if(self->color_model)
    g_object_unref(G_OBJECT(self->color_model));

  if(self->tile)
    g_object_unref(G_OBJECT(self->tile));

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

GeglColorModel * 
gegl_image_impl_color_model (GeglImageImpl * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_IMPL (self), NULL);
	
  return self->color_model;
}

void 
gegl_image_impl_set_color_model (GeglImageImpl * self, 
                                   GeglColorModel * cm)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_IMPL (self));

  /* Unref any old one */
  if(self->color_model)
    g_object_unref(self->color_model);

  /* Set the new color model */
  self->color_model = cm;
  if(cm)
    g_object_ref(cm);
}
