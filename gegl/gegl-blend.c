#include "gegl-blend.h"
#include "gegl-attributes.h"
#include "gegl-scanline-processor.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_OPACITY,
  PROP_LAST 
};

static void class_init (GeglBlendClass * klass);
static void init (GeglBlend * self, GeglBlendClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_blend_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBlendClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBlend),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_COMP , 
                                     "GeglBlend", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglBlendClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OPACITY,
                                   g_param_spec_float ("opacity",
                                                       "Opacity",
                                                       "Opacity of this blend.",
                                                       0.0, 
                                                       1.0,
                                                       1.0,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_CONSTRUCT));
}

static void 
init (GeglBlend * self, 
      GeglBlendClass * klass)
{
  self->opacity = 1.0;
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglBlend *blend = GEGL_BLEND (gobject);
  switch (prop_id)
  {
    case PROP_OPACITY:
      g_value_set_float(value, gegl_blend_get_opacity(blend));  
      break;
    default:
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglBlend *blend = GEGL_BLEND (gobject);
  switch (prop_id)
  {
    case PROP_OPACITY:
      gegl_blend_set_opacity(blend, g_value_get_float(value));  
      break;
    default:
      break;
  }
}


/**
 * gegl_blend_get_opacity:
 * @self: a #GeglBlend.
 *
 * Gets the opacity.
 *
 * Returns: the opacity. 
 **/
gfloat
gegl_blend_get_opacity (GeglBlend * self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_BLEND (self), FALSE);

  return self->opacity;
}


/**
 * gegl_blend_set_opacity:
 * @self: a #GeglBlend.
 * @opacity: the opacity. 
 *
 * Sets the opacity. 
 *
 **/
void 
gegl_blend_set_opacity (GeglBlend * self, 
                        gfloat opacity)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BLEND (self));

  self->opacity = opacity;
}
