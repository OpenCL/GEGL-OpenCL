#include "gegl-sampled-image.h"
#include "gegl-image.h"
#include "gegl-image-mgr.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_LAST 
};

static void class_init (GeglSampledImageClass * klass);
static void init(GeglSampledImage *self, GeglSampledImageClass *klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void compute_have_rect (GeglImage * self_image, GeglRect *have_rect, GList * input_have_rects);
static void compute_result_rect (GeglImage * self_image, GeglRect * result_rect, GeglRect * need_rect, GeglRect *have_rect);

static gpointer parent_class = NULL;

GType
gegl_sampled_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglSampledImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglSampledImage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE, 
                                     "GeglSampledImage", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglSampledImageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglImageClass *image_class = GEGL_IMAGE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);
   
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  image_class->compute_have_rect = compute_have_rect;
  image_class->compute_result_rect = compute_result_rect;

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                      "Width",
                                                      "GeglSampledImage width",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                      "Height",
                                                      "GeglSampledImage height",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  return;
}

static void 
init (GeglSampledImage * self, 
      GeglSampledImageClass * klass)
{
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglImage *self_image = GEGL_IMAGE(gobject);

  GeglImageMgr *mgr = gegl_image_mgr_instance();
  gegl_image_mgr_add_image(mgr, self_image);
  g_object_unref(mgr);

  return gobject;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSampledImage *self = GEGL_SAMPLED_IMAGE (gobject);

  switch (prop_id)
  {
    case PROP_WIDTH:
      if(!GEGL_OBJECT(gobject)->constructed)
        gegl_sampled_image_set_width(self,g_value_get_int (value));
      else
        g_message("Cant set width after construction");
      break;
    case PROP_HEIGHT:
      if(!GEGL_OBJECT(gobject)->constructed)
        gegl_sampled_image_set_height(self, g_value_get_int (value));
      else
        g_message("Cant set height after construction");
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
  GeglSampledImage *self = GEGL_SAMPLED_IMAGE (gobject);

  switch (prop_id)
  {
    case PROP_WIDTH:
      g_value_set_int (value, gegl_sampled_image_get_width(self));
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, gegl_sampled_image_get_height(self));
      break;
    default:
      break;
  }
}

/**
 * gegl_sampled_image_get_width:
 * @self: a #GeglSampledImage. 
 *
 * Gets the width of the sampled image.
 *
 * Returns: width of the sampled image.
 **/
gint 
gegl_sampled_image_get_width (GeglSampledImage * self)
{
  return self->width;
}

void
gegl_sampled_image_set_width (GeglSampledImage * self, 
                                    gint width)
{
  self->width = width;
}

/**
 * gegl_sampled_image_get_height:
 * @self: a #GeglSampledImage. 
 *
 * Gets the height of the sampled image.
 *
 * Returns: height of the sampled image.
 **/
gint 
gegl_sampled_image_get_height (GeglSampledImage * self)
{
  return self->height;
}

void
gegl_sampled_image_set_height (GeglSampledImage * self, 
                               gint height)
{
  self->height = height;
}

static void 
compute_have_rect (GeglImage * self_image, 
                   GeglRect * have_rect,
                   GList * input_have_rects)
{
  GeglSampledImage * self = GEGL_SAMPLED_IMAGE(self_image);
  have_rect->x = 0;
  have_rect->y = 0;
  have_rect->w = self->width;
  have_rect->h = self->height;
}

static void 
compute_result_rect (GeglImage * self_image, 
                     GeglRect * result_rect, 
                     GeglRect * need_rect,
                     GeglRect * have_rect)
{
  gegl_rect_intersect (result_rect, need_rect, have_rect);
}
