#include "gegl-tile-mgr.h"    
#include "gegl-sampled-image.h"
#include "gegl-color-model.h"
#include "gegl-tile.h"
#include "gegl-utils.h"
#include "gegl-mem-buffer.h"    

static void class_init (GeglTileMgrClass * klass);
static void init (GeglTileMgr * self, GeglTileMgrClass * klass);
static void finalize (GObject * self_object);

static GeglTile * fetch_output_tile (GeglTileMgr * self, GeglOp * node, GeglImage * dest, GeglRect * area);
static GeglTile * fetch_input_tile (GeglTileMgr * self, GeglImage * input, GeglRect * area);
static GeglTile * make_tile (GeglTileMgr * self, GeglImage* image, GeglRect * area);
static void release_tiles (GeglTileMgr * self, GList * request_list);
static GeglTile * get_tile (GeglTileMgr * self, GeglImage * image);
static GeglBuffer * create_buffer (GeglTileMgr * self, GeglTile *tile);

static gpointer parent_class = NULL;

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

  klass->fetch_output_tile = fetch_output_tile;
  klass->fetch_input_tile = fetch_input_tile;
  klass->release_tiles = release_tiles;
  klass->get_tile = get_tile;
  klass->make_tile = make_tile;
  klass->create_buffer = create_buffer;

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
 * gegl_tile_mgr_fetch_output_tile:
 * @self: a #GeglTileMgr. 
 * @op: #GeglOp we are computing. 
 * @dest: #GeglImage to use to get tiles. 
 * @area: #GeglRect of tile needed.
 *
 * Fetch a tile for the passed image and area.
 *
 * Returns: an output #GeglTile for this image containing the area.
 **/
GeglTile * 
gegl_tile_mgr_fetch_output_tile (GeglTileMgr * self, 
                                 GeglOp * op, 
                                 GeglImage* dest, 
                                 GeglRect * area)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);

  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->fetch_output_tile)
      return (*klass->fetch_output_tile)(self,op,dest,area);
    else
      return NULL; 
  }
}

static GeglTile * 
fetch_output_tile (GeglTileMgr * self, 
                   GeglOp * op, 
                   GeglImage * dest, 
                   GeglRect * area)
{
  GeglTile *tile = (GeglTile*) dest->tile; 

  if(!tile)
    {
      LOG_DEBUG("fetch_output_tile", 
                "Didnt find a tile for this dest, make one");
      tile = make_tile(self, dest, area);
    }
  else 
    {
      GeglColorModel *dest_color_model = gegl_image_color_model(dest);
      GeglColorModel *tile_color_model = gegl_tile_get_color_model(tile);
      GeglRect tile_rect;
      gegl_tile_get_area(tile, &tile_rect);

      LOG_DEBUG("fetch_output_tile", 
                "Found output tile for dest");

      if(GEGL_IS_SAMPLED_IMAGE(dest)) 
        {
          LOG_DEBUG("fetch_output_tile",
                    "dest is an SampledImage");
          if(!gegl_rect_contains(&tile_rect, area))
            {
              LOG_INFO("fetch_output_tile",
                        "Dest SampledImage tile doesnt contain requested area.");
              return NULL;
            }

          if(tile_color_model != dest_color_model)
            {
              LOG_INFO("fetch_output_tile",
                       "Fetched Tile and Dest(Image) have different color models.");

              return NULL;
            }
        }
      else 
        {
          LOG_DEBUG("fetch_output_tile",
                    "Dest is regular Image, not an SampledImage.");

          if(!gegl_rect_equal(&tile_rect, area))
            {
              LOG_DEBUG("fetch_output_tile",
                        "Size changed, re-alloc the tile for Image");

              gegl_tile_alloc(tile, area, dest_color_model); 
            }

          if (tile_color_model != dest_color_model)
            {
              LOG_DEBUG("fetch_output_tile",
                        "ColorModel changed ,re-alloc the tile");

              gegl_tile_alloc(tile, area, dest_color_model); 
            }
        }
    }

  /* If tile is not NULL, validate the data. */
  if(tile)
    gegl_tile_validate_data(tile);

  return tile;
}

/**
 * gegl_tile_mgr_fetch_input_tile:
 * @self: a #GeglTileMgr. 
 * @input: #GeglOp we are computing. 
 * @area: #GeglRect needed.
 *
 * Fetch a tile for the passed image and area.
 *
 * Returns: an input #GeglTile for this image containing the area.
 **/
GeglTile * 
gegl_tile_mgr_fetch_input_tile (GeglTileMgr * self, 
                                GeglImage * input, 
                                GeglRect * area)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);

  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->fetch_input_tile)
      return (*klass->fetch_input_tile)(self,input,area);
    else
      return NULL;
  }
}

static GeglTile * 
fetch_input_tile (GeglTileMgr * self, 
                  GeglImage * input, 
                  GeglRect * area)
{
  GeglTile *tile = input->tile; 

  /* If this doesnt find the input, we are in trouble */ 
  if(!tile)
    LOG_DEBUG("fetch_input_tile", 
              "didnt find input tile");

  return tile;
}

/**
 * gegl_tile_mgr_release_tile:
 * @self: a #GeglTileMgr. 
 * @tile: #GeglTile. 
 *
 * Release the passed tile after image processing.
 *
 **/
void 
gegl_tile_mgr_release_tiles (GeglTileMgr * self, 
                             GList * request_list)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE_MGR (self));
  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->release_tiles)
      (*klass->release_tiles)(self,request_list);
  }
}

static void 
release_tiles (GeglTileMgr * self, 
               GList * request_list)
{
  LOG_DEBUG("release_tiles", "doing nothing on release_tiles");
}

GeglTile *
gegl_tile_mgr_get_tile (GeglTileMgr * self, 
                       GeglImage * image)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->get_tile)
      return (*klass->get_tile)(self,image);
    else
      return NULL;
  }
}

GeglTile *
get_tile (GeglTileMgr * self, 
          GeglImage * image)
{
  return image->tile;
}

GeglBuffer *  
gegl_tile_mgr_create_buffer (GeglTileMgr * self, 
                             GeglTile * tile)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->create_buffer)
      return (*klass->create_buffer)(self,tile);
    else
      return NULL;
  }
}

static GeglBuffer * 
create_buffer (GeglTileMgr * self, 
               GeglTile * tile)
{
  gint bytes_per_buffer;
  GeglColorModel * color_model = gegl_tile_get_color_model(tile);
  gint bytes_per_channel = gegl_color_model_bytes_per_channel(color_model);
  gint num_buffers = gegl_color_model_num_channels(color_model);
  GeglRect area;

  gegl_tile_get_area(tile, &area);
  bytes_per_buffer = area.w * area.h * bytes_per_channel;

  return g_object_new (GEGL_TYPE_MEM_BUFFER, 
                       "bytesperbuffer", bytes_per_buffer, 
                       "numbuffers", num_buffers,
                        NULL);  
}

GeglTile *  
gegl_tile_mgr_make_tile (GeglTileMgr * self, 
                         GeglImage* image, 
                         GeglRect * area)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), NULL);
  {
    GeglTileMgrClass *klass = GEGL_TILE_MGR_GET_CLASS(self);

    if(klass->make_tile)
      return (*klass->make_tile)(self,image,area);
    else
      return NULL;
  }
}

static GeglTile * 
make_tile (GeglTileMgr * self, 
           GeglImage* image, 
           GeglRect * area)
{
  g_return_val_if_fail (self != NULL, (GeglTile * )0);
  g_return_val_if_fail (GEGL_IS_TILE_MGR (self), (GeglTile * )0);

  /* Create tile. */
  image->tile = g_object_new (GEGL_TYPE_TILE, 
                              "area", area, 
                              "colormodel", gegl_image_color_model(image),
                              NULL);  


  LOG_DEBUG("make_tile", 
            "making tile %x for image %x", 
            (guint)image->tile, (guint)image);

  return image->tile;
}
