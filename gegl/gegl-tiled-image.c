#include "gegl-tiled-image.h"
#include "gegl-image.h"
#include "gegl-tile.h"
#include "gegl-tile-mgr.h"
#include "gegl-color-model.h"
#include "gegl-object.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_TILE,
  PROP_LAST 
};

static void class_init (GeglTiledImageClass * klass);
static void init(GeglTiledImage *self, GeglTiledImageClass *klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void compute_have_rect(GeglFilter * op, GeglRect *have_rect, GList * input_have_rects);
static GeglColorModel *compute_derived_color_model (GeglFilter * op, GList* input_color_models);

static gpointer parent_class = NULL;

GType
gegl_tiled_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglTiledImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglTiledImage),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE, 
                                     "GeglTiledImage", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglTiledImageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);
   
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->compute_have_rect = compute_have_rect;
  filter_class->compute_derived_color_model = compute_derived_color_model;

   g_object_class_install_property (gobject_class, PROP_TILE,
                                   g_param_spec_object ("tile",
                                                        "Tile",
                                                        "The GeglTiledImage s tile",
                                                         GEGL_TYPE_TILE,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglTiledImage * self, 
      GeglTiledImageClass * klass)
{
  g_object_set(self, "num_inputs", 0, NULL);
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglImage *image = GEGL_IMAGE(gobject);
  GeglTile *tile = gegl_image_get_tile(image); 
  GeglColorModel * color_model = gegl_tile_get_color_model(tile);
  GeglTileMgr *mgr = gegl_tile_mgr_instance();

  gegl_image_set_color_model(image, color_model);
  gegl_tile_mgr_validate_data(mgr, tile);

  return gobject;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{

  switch (prop_id)
  {
    case PROP_TILE:
      {
        GeglImage *image = GEGL_IMAGE (gobject);
        gegl_image_set_tile (image, g_value_get_tile(value));
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
  switch (prop_id)
  {
    case PROP_TILE:
      {
         GeglImage *image = GEGL_IMAGE (gobject);
         g_value_set_tile (value, gegl_image_get_tile(image));
      }
      break;
    default:
      break;
  }
}

static void 
compute_have_rect(GeglFilter * filter, 
                  GeglRect *have_rect, 
                  GList * input_have_rects)
{ 
  GeglImage * image = GEGL_IMAGE(filter);
  GeglTile * tile = gegl_image_get_tile(image);
  GeglRect rect;

  gegl_tile_get_area(tile, &rect);
  gegl_rect_copy(have_rect, &rect); 
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                             GList * input_color_models)
{
  return gegl_image_color_model(GEGL_IMAGE(filter)); 
}
