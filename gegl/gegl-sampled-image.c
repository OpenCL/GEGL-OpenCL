#include "gegl-sampled-image.h"
#include "gegl-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

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

static void compute_have_rect(GeglOp *self, GList * input_values);

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
  GeglOpClass *op_class = GEGL_OP_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);
   
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  op_class->compute_have_rect = compute_have_rect;

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
  GeglNode * node = GEGL_NODE(self);
  node->num_inputs = 0;
  gegl_node_set_num_outputs(node, 1);
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglSampledImage *self = GEGL_SAMPLED_IMAGE(gobject);
  GeglImage *image = GEGL_IMAGE(gobject);
  GeglColorModel * color_model = gegl_image_color_model(image);
  GeglTileMgr *mgr = gegl_tile_mgr_instance();
  GeglRect rect; 

  gegl_rect_set(&rect, 0,0,self->width,self->height);

  LOG_DEBUG("sampled_image_constructor", "creating tile for %s %x", 
            G_OBJECT_TYPE_NAME(self), (guint)self);
  image->tile = gegl_tile_mgr_create_tile(mgr, color_model, &rect); 

  LOG_DEBUG("sampled_image_constructor", "validating tile %x for %s %x", 
            image->tile, G_OBJECT_TYPE_NAME(self), (guint)self);
  gegl_tile_mgr_validate_data(mgr, image->tile);

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
compute_have_rect(GeglOp *op, 
                  GList * input_values)
{
  GeglSampledImage * self = GEGL_SAMPLED_IMAGE(op);
  GeglRect have_rect;
  GeglRect need_rect;
  GeglRect result_rect;

  have_rect.x = 0;
  have_rect.y = 0;
  have_rect.w = self->width;
  have_rect.h = self->height;

  /* Get the need rect. */
  g_value_get_image_data_rect(op->output_value, &need_rect);

  /* Find the result rect. */
  gegl_rect_intersect(&result_rect, &have_rect, &need_rect);

  /* Store the result rect in the output value. */
  g_value_set_image_data_rect(op->output_value, &result_rect);
}
