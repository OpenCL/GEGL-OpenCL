#include "gegl-sampled-image.h"
#include "gegl-sampled-image-impl.h"
#include "gegl-image-mgr.h"
#include "gegl-color-model.h"
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
  GeglOp *self_op = GEGL_OP(self);
  self_op->op_impl = g_object_new(GEGL_TYPE_SAMPLED_IMAGE_IMPL, NULL);  
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglOp *self_op = GEGL_OP(gobject);

  GeglImageMgr *mgr = gegl_image_mgr_instance();
  gegl_image_mgr_add_image(mgr, GEGL_IMAGE_IMPL(self_op->op_impl));
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
 * Gets the width of the image buffer.
 *
 * Returns: width of the image buffer.
 **/
gint 
gegl_sampled_image_get_width (GeglSampledImage * self)
{
    GeglSampledImageImpl * sampled_image_impl = 
      GEGL_SAMPLED_IMAGE_IMPL(GEGL_OP(self)->op_impl);
    return gegl_sampled_image_impl_get_width(sampled_image_impl);
}

void
gegl_sampled_image_set_width (GeglSampledImage * self, 
                             gint width)
{
  GeglSampledImageImpl * sampled_image_impl = 
    GEGL_SAMPLED_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  gegl_sampled_image_impl_set_width (sampled_image_impl, width);
}

/**
 * gegl_sampled_image_get_height:
 * @self: a #GeglSampledImage. 
 *
 * Gets the height of the image buffer.
 *
 * Returns: height of the image buffer.
 **/
gint 
gegl_sampled_image_get_height (GeglSampledImage * self)
{
  GeglSampledImageImpl * sampled_image_impl = 
    GEGL_SAMPLED_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  return gegl_sampled_image_impl_get_height(sampled_image_impl);
}

void
gegl_sampled_image_set_height (GeglSampledImage * self, 
                              gint height)
{
  GeglSampledImageImpl * sampled_image_impl = 
    GEGL_SAMPLED_IMAGE_IMPL(GEGL_OP(self)->op_impl);
  gegl_sampled_image_impl_set_height(sampled_image_impl, height);
}

static void 
compute_have_rect (GeglImage * self_image, 
                   GeglRect * have_rect,
                   GList * input_have_rects)
{
  GeglOp * self_op = GEGL_OP(self_image);
  GeglSampledImageImpl * sampled_image_impl = GEGL_SAMPLED_IMAGE_IMPL(self_op->op_impl);
  have_rect->x = 0;
  have_rect->y = 0;
  have_rect->w = gegl_sampled_image_impl_get_width(sampled_image_impl);
  have_rect->h = gegl_sampled_image_impl_get_height(sampled_image_impl);
}

static void 
compute_result_rect (GeglImage * self_image, 
                     GeglRect * result_rect, 
                     GeglRect * need_rect,
                     GeglRect * have_rect)
{
  gegl_rect_intersect (result_rect, need_rect, have_rect);
}
