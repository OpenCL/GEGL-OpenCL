#include "gegl-tile-mgr.h"    
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-utils.h"
#include "gegl-mem-buffer.h"    

static void class_init (GeglTileMgrClass * klass);
static void init (GeglTileMgr * self, GeglTileMgrClass * klass);
static void finalize (GObject * self_object);

static gpointer parent_class = NULL;

static GeglTileMgr *tile_mgr_singleton = NULL;

void 
gegl_tile_mgr_install(GeglTileMgr *tile_mgr)
{
  g_return_if_fail (tile_mgr != NULL);
  g_return_if_fail (GEGL_IS_TILE_MGR (tile_mgr));

  /* Install this tile manager */
  tile_mgr_singleton = tile_mgr;
} 

GeglTileMgr *
gegl_tile_mgr_instance()
{
  if(!tile_mgr_singleton)
    {
      g_warning("No TileMgr installed.");
      return NULL;
    }

  g_object_ref(tile_mgr_singleton);
  return tile_mgr_singleton;
} 

GType
gegl_tile_mgr_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglTileMgrClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglTileMgr),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglTileMgr", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglTileMgrClass * klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  g_object_class->finalize = finalize;

  return;
}
    
static void 
init (GeglTileMgr * self, 
      GeglTileMgrClass * klass)
{
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_tile_mgr_validate_tile:
 * @self: a #GeglTileMgr. 
 * @tile: #GeglTile we are validating. 
 * @area: #GeglRect needed.
 *
 * Check if tile is valid for the desired area. Realloc the tile if it doesnt
 * contain the rect area. Then validate the data for the tile as well.
 *
 * Returns: a valid #GeglTile for this image containing the area.
 **/
GeglTile * 
gegl_tile_mgr_validate_tile (GeglTileMgr * self, 
                             GeglTile * tile, 
                             GeglRect * area)
{
  GeglTile *valid_tile;
  GeglRect tile_rect;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  g_return_val_if_fail (tile != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE(tile), NULL);
  g_return_val_if_fail (area != NULL, NULL);

  gegl_tile_get_area(tile, &tile_rect);

  if(gegl_rect_contains(&tile_rect, area))
    valid_tile = tile;
  else 
    {
      GeglColorModel * color_model = gegl_tile_get_color_model(tile);
      g_object_unref(tile);
      LOG_DEBUG("tile_mgr_validate_tile", "have to re-create tile %x", (guint)tile);
      valid_tile = gegl_tile_mgr_create_tile (self, color_model, area);
      LOG_DEBUG("tile_mgr_validate_tile", "new tile is %x", (guint)valid_tile);
    }
   
  gegl_tile_mgr_validate_data(self, valid_tile);

  return valid_tile;
}

/**
 * gegl_tile_mgr_validate_data:
 * @self: a #GeglTileMgr. 
 * @tile : #GeglTile we are validating data for. 
 *
 * Validate the data for the tile.
 *
 **/
void
gegl_tile_mgr_validate_data (GeglTileMgr * self, 
                             GeglTile * tile)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_MGR (self));
  g_return_if_fail (tile != NULL);

  /* Make sure the tile data is there. */
  /*
  LOG_DEBUG("tile_mgr_validate_data", "validating data for tile %x", (guint)tile);
  */
  gegl_tile_validate_data(tile);
}

/**
 * gegl_tile_mgr_create_buffer:
 * @self: a #GeglTileMgr. 
 * @tile : #GeglTile we are creating a buffer for. 
 *
 * Returns: a #GeglBuffer for this tile.
 **/
GeglBuffer *  
gegl_tile_mgr_create_buffer (GeglTileMgr * self, 
                             GeglTile * tile)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  g_return_val_if_fail (tile != NULL, NULL);

    {
      gint bytes_per_buffer;
      GeglColorModel * color_model = gegl_tile_get_color_model(tile);
      gint bytes_per_channel = gegl_color_model_bytes_per_channel(color_model);
      gint num_buffers = gegl_color_model_num_channels(color_model);
      GeglRect area;

      gegl_tile_get_area(tile, &area);
      bytes_per_buffer = area.w * area.h * bytes_per_channel;

      /*
      LOG_DEBUG("tile_mgr_create_buffer", "creating mem buffer for tile %x", (guint)tile);
      */
      return g_object_new (GEGL_TYPE_MEM_BUFFER, 
                           "bytesperbuffer", bytes_per_buffer, 
                           "numbuffers", num_buffers,
                            NULL);  
    }
}

/**
 * gegl_tile_mgr_create_tile:
 * @self: a #GeglTileMgr. 
 * @color_model : #GeglColorModel.  
 *
 * Returns: a #GeglBuffer for this tile.
 **/
GeglTile *  
gegl_tile_mgr_create_tile (GeglTileMgr * self, 
                           GeglColorModel* color_model, 
                           GeglRect * area)
{
  GeglTile * tile;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  g_return_val_if_fail (color_model != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_MODEL (color_model), NULL);
  g_return_val_if_fail (area != NULL, NULL);

  /* Create tile. */
  tile = g_object_new (GEGL_TYPE_TILE, 
                       "area", area, 
                       "colormodel", color_model,
                       NULL);  
  LOG_DEBUG("tile_mgr_create_tile", "creating tile %x", (guint)tile);


  return tile;
}
