#include "gegl-simple-image-mgr.h"
#include "gegl-image.h"    
#include "gegl-tile-mgr.h"    
#include "gegl-sampled-image.h"
#include "gegl-sampled-image-impl.h"
#include "gegl-mem-buffer.h"
#include "gegl-image-impl.h"    
#include "gegl-tile.h"
#include "gegl-utils.h"

static void class_init (GeglSimpleImageMgrClass * klass);
static void init (GeglSimpleImageMgr * self, GeglSimpleImageMgrClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize (GObject * self_object);
static void apply (GeglImageMgr * self_image_mgr, GeglOp * root, GeglSampledImage * dest, GeglRect * roi);
static gboolean breadth_first_visit (GeglNode * node, gpointer data);
static gboolean depth_first_visit (GeglNode * node, gpointer data);
static void compute_op (GeglSimpleImageMgr * self, GeglOp * op);
static void set_request (GeglSimpleImageMgr * self, GeglOpRequest * request, GeglRect * rect, GeglTile * tile);

static GeglImage * find_image_input(GeglNode * input); 
static GList * find_image_inputs(GeglNode *node);

static GList *make_input_have_rects_list (GList *inputs);
static void free_input_have_rects_list(GList *have_rects);
static GList * make_input_color_models_list (GList *inputs);
static void free_input_color_models_list(GList *color_models);

static void add_image (GeglImageMgr * self_image_mgr, GeglImageImpl * image_impl);

static gpointer parent_class = NULL;
static GeglSimpleImageMgr * simple_image_mgr_singleton = NULL;

GType
gegl_simple_image_mgr_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglSimpleImageMgrClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglSimpleImageMgr),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_IMAGE_MGR, 
                                     "GeglSimpleImageMgr", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglSimpleImageMgrClass * klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GeglImageMgrClass *image_mgr_class = GEGL_IMAGE_MGR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  g_object_class->finalize = finalize;
  g_object_class->constructor = constructor;

  image_mgr_class->apply = apply;
  image_mgr_class->add_image = add_image;

  return;
}

static void 
init (GeglSimpleImageMgr      *self, 
      GeglSimpleImageMgrClass *klass)
{
  GeglImageMgr *self_image_mgr = GEGL_IMAGE_MGR(self);
  self_image_mgr->tile_mgr = g_object_new(GEGL_TYPE_TILE_MGR,NULL);
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject;

  if(!simple_image_mgr_singleton)
    {
      gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
      simple_image_mgr_singleton = GEGL_SIMPLE_IMAGE_MGR(gobject); 
    }
  else
    {
      gobject = G_OBJECT(simple_image_mgr_singleton); 
      g_object_ref(gobject);
      g_object_freeze_notify(gobject);
    }

  return gobject;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
apply (GeglImageMgr      *self_image_mgr, 
       GeglOp            *root, 
       GeglSampledImage  *dest, 
       GeglRect          *roi)
{
  GeglSimpleImageMgr *self = GEGL_SIMPLE_IMAGE_MGR(self_image_mgr);
  GeglOpImpl * op_impl = gegl_op_get_op_impl(root);
  GeglImageImpl * dest_image_impl = NULL;
  g_return_if_fail(root);

  LOG_DEBUG("apply", 
            "Top of apply: root op: %s(%x) op_impl: %s(%x)", 
            G_OBJECT_TYPE_NAME(root),
            (guint)root, 
            G_OBJECT_TYPE_NAME(op_impl),
            (guint)op_impl);

  g_object_unref(op_impl);

  /* Figure out what roi to use. */
  if (roi)
    gegl_rect_copy(&self->roi,roi);
  else if(dest)
    {
      gint width = gegl_sampled_image_get_width(dest);
      gint height = gegl_sampled_image_get_height(dest);
      gegl_rect_set(&self->roi,0,0,width,height);
    }
  else
    gegl_rect_set(&self->roi,0,0,G_MAXINT,G_MAXINT);


  /* Set the need rect and dest of the root image op. */ 
  {
    GeglImage *root_image = find_image_input(GEGL_NODE(root));
    gegl_image_set_need_rect(root_image,&self->roi);

    /* Set the dest of the root */
    if(dest)
      {
        dest_image_impl = GEGL_IMAGE_IMPL(gegl_op_get_op_impl(GEGL_OP(dest)));
        gegl_image_set_dest(root_image,dest_image_impl);
        g_object_unref(dest_image_impl);
      }
    else
      gegl_image_set_dest(root_image,NULL);
  }

  LOG_DEBUG("apply", 
            "Beginning breadth first traversal"); 

  /* Traverse breadth first, to init derived color models. */
  gegl_node_traverse_breadth_first(GEGL_NODE(root), 
                                   breadth_first_visit, 
                                   (gpointer)self);

  LOG_DEBUG("apply", 
            "Beginning depth first traversal"); 

  /* Traverse depth first, compute have rects, colormodels. */
  gegl_node_traverse_depth_first(GEGL_NODE(root),
                                 depth_first_visit,
                                 (gpointer)self,
                                 TRUE);
}

static GList *
find_image_inputs(GeglNode *node)
{
  GList * image_inputs = NULL; 
  GList * inputs = gegl_node_get_inputs(node);
  GList * llink = g_list_first(inputs);

  while(llink)
    {
      GeglNode *input = (GeglNode*)llink->data;
      GeglImage * image_input = find_image_input(input);
      image_inputs = g_list_append(image_inputs, image_input);

      llink = g_list_next(llink);
    }

  return image_inputs;
}

static GeglImage *
find_image_input(GeglNode * input) 
{
  GeglNode * node = input;

  while(node && !GEGL_IS_IMAGE(node))
    {
      gint inheriting_input = gegl_node_inheriting_input(node);
      node = gegl_node_get_nth_input(node, inheriting_input); 
    }

  return (GeglImage*)node;
}

/**
 * breadth_first_visit:
 * @node: #GeglNode to process.
 * @data: the #GeglSimpleImageMgr.
 *
 * Compute need rects and initialize the derived color model flag to NULL for
 * each node.
 *
 * Returns: FALSE to continue traversal. 
 **/
static gboolean 
breadth_first_visit (GeglNode *node, 
                     gpointer  data)
{
  LOG_DEBUG("breadth_first_visit", 
            "node: %s(%x)", 
             G_OBJECT_TYPE_NAME(node), (guint)node);

  if(GEGL_IS_IMAGE(node)) 
    { 
      gint i;
      GeglImage * image = GEGL_IMAGE(node);
      GList * image_inputs = find_image_inputs(node); 
      gint num_inputs = g_list_length(image_inputs);

      LOG_DEBUG("breadth_first_visit", 
                "It's an Image node");

      /* Compute the need rect of each image input. */
      for(i = 0; i < num_inputs; i++)
        {
          GeglRect preimage_rect;
          GeglRect need_rect;
          GeglImage *image_input = GEGL_IMAGE(g_list_nth_data(image_inputs,i));

          LOG_DEBUG("breadth_first_visit", 
                    "Computing need rect for %dth input image node: %s(%x)", 
                     i, G_OBJECT_TYPE_NAME(image_input),(guint)image_input);

          /* Get the need rect of the current image op. */
          gegl_image_get_need_rect(image, &need_rect);

          /* Get the preimage of the need rect on ith input */
          gegl_image_compute_preimage(image, &preimage_rect, &need_rect, i);

          /* Set the need_rect of the ith input */
          gegl_image_set_need_rect(image_input, &preimage_rect);

          LOG_DEBUG("breadth_first_visit",
                    "Computed preimage_rect for %d th input: %d %d %d %d", i, 
                     preimage_rect.x, preimage_rect.y, preimage_rect.w, preimage_rect.h);

        }

      /* Free the inputs list. */
      if(image_inputs)
        g_list_free(image_inputs);
    }
  else
    {
      LOG_DEBUG("breadth_first_visit", 
                "It's not an Image node, nothing to do");
    }

  return FALSE;
}

static GList * 
make_input_have_rects_list (GList *inputs)
{
  GList *llink = inputs; 
  GList *have_rects = NULL;

  while(llink)
    {
      GeglImage * image = (GeglImage*)llink->data;
      GeglRect * have_rect = g_new(GeglRect, 1);

      /* Retrieve the have rect of this op */
      gegl_image_get_have_rect(image, have_rect);
      have_rects = g_list_append (have_rects, have_rect); 
      llink = llink->next;
    }

  return have_rects;
}

static void
free_input_have_rects_list(GList *have_rects)
{
  GList *llink = have_rects; 

  while(llink)
    {
      GeglRect * have_rect = (GeglRect*)llink->data;

      /* Free all the data of the list. */
      g_free(have_rect);
      llink = llink->next;
    }

  if(have_rects)
    g_list_free(have_rects);
}

static GList * 
make_input_color_models_list (GList *inputs)
{
  GList *llink = inputs; 
  GList *color_models = NULL;

  while(llink)
    {
      GeglColorModel * cm = NULL;
      GeglImage * image = (GeglImage*)llink->data;
      cm = gegl_image_color_model(image);
      color_models = g_list_append (color_models, cm); 
      llink = llink->next;

    }

  return color_models;
}

static void
free_input_color_models_list(GList *color_models)
{
  if(color_models)
    g_list_free(color_models);
}

/**
 * depth_first_visit:
 * @node: #GeglNode to process.
 * @data: the #GeglSimpleImageMgr.
 *
 * Computes have rects, result rects, derived color models and then the
 * result for each @node.  
 *
 * Returns: FALSE to continue traversal. 
 **/
static gboolean 
depth_first_visit (GeglNode *node, 
                   gpointer  data)
{
  GeglSimpleImageMgr *self = (GeglSimpleImageMgr*)data;

  LOG_DEBUG("depth_first_visit", 
            "node %x type %s num_inputs %d", 
            (guint)node, G_OBJECT_TYPE_NAME(node),
            g_list_length(gegl_node_get_inputs(node)));

  if(GEGL_IS_IMAGE(node)) 
  {
    GeglImage * image = GEGL_IMAGE(node);
    GList * image_inputs = find_image_inputs(node); 
    GeglRect have_rect;
    GeglRect need_rect;
    GeglRect result_rect;

    GList * input_have_rects = make_input_have_rects_list(image_inputs);
    GList * input_color_models = make_input_color_models_list(image_inputs);

    LOG_DEBUG("depth_first_visit", 
              "It's an Image node");

    /* Compute the have rect */
    gegl_image_compute_have_rect(image, &have_rect, input_have_rects);
    free_input_have_rects_list(input_have_rects);
    gegl_image_set_have_rect(image, &have_rect);

    /* Compute the result rect */
    gegl_image_get_need_rect(image,&need_rect);
    gegl_image_compute_result_rect(image, &result_rect, &need_rect, &have_rect);
    gegl_image_set_result_rect(image,&result_rect);

    /* Compute the derived color model */
    gegl_image_compute_derived_color_model(image, input_color_models);
    free_input_color_models_list(input_color_models);

    /* Free the list of image inputs. */
    if(image_inputs)
      g_list_free(image_inputs);
  }
  else
  {
    LOG_DEBUG("depth_first_visit", 
              "It's not an Image node, nothing to do");
  }

  /* Do the op */
  compute_op(self, GEGL_OP(node));


  return FALSE;
}

/**
 * compute_op:
 * @self: a #GeglSimpleImageMgr 
 * @op: #GeglOp to compute.
 *
 * Retrieve the tiles to use for dest and sources for this op.
 * Set up an #GeglOpRequest for each, and pass the list of requests to @op
 * in prepare, process, and finish routines.  Release the tiles after
 * processing. 
 *
 **/
static void 
compute_op (GeglSimpleImageMgr *self, 
            GeglOp             *op)
{
  gint i;
  GeglRect result_rect;
  GeglTile *output_tile;
  GeglTileMgr * tile_mgr = GEGL_IMAGE_MGR(self)->tile_mgr; 
  GList *request_list = NULL;
  GeglOpImpl *op_impl = gegl_op_get_op_impl(op);
  GList * image_inputs = find_image_inputs(GEGL_NODE(op)); 
  gint num_inputs = g_list_length(image_inputs);
  GeglOpRequest *requests;
  gint first_source_index;

  LOG_DEBUG("compute_op",
            "op %x type %s op_impl %x type %s", 
            (guint)op, 
             G_OBJECT_TYPE_NAME(op),
            (guint)op_impl, 
             G_OBJECT_TYPE_NAME(op_impl));

  if(GEGL_IS_IMAGE(op))
    {
      GeglImageImpl *output_image_impl = NULL;
      GeglImageImpl *dest = NULL;
      requests = g_new(GeglOpRequest,num_inputs+1);
      first_source_index = 1;

      dest = gegl_image_get_dest(GEGL_IMAGE(op));

      if (!dest)
        output_image_impl = GEGL_IMAGE_IMPL(op_impl);
      else
        output_image_impl = dest;

      LOG_DEBUG("compute_op",
                "output image op_impl %x type %s",  
                (guint)output_image_impl,
                G_OBJECT_TYPE_NAME(output_image_impl));  

      LOG_DEBUG("compute_op", 
                "fetching dest tile");

      gegl_image_get_result_rect(GEGL_IMAGE(op), &result_rect);

      /* Fetch the output tile */
      output_tile = gegl_tile_mgr_fetch_output_tile(tile_mgr, 
                                                    op, 
                                                    output_image_impl, 
                                                    &result_rect);

      if(dest)
        g_object_unref(dest);

      LOG_DEBUG("compute_op", 
                "output tile is %x", 
                (guint)output_tile);

      set_request(self, &requests[0], &result_rect, output_tile);

      /* Put request on the list of requests to pass back to op. */
      request_list = g_list_append(request_list, (gpointer)&requests[0]);
    }
  else
    {
      requests = g_new(GeglOpRequest,num_inputs);
      first_source_index = 0;
    }

  for(i = 0 ; i < num_inputs; i++)
    {
      GeglImage *image_input = (GeglImage*)g_list_nth_data(image_inputs,i);
      GeglImageImpl *input_image_impl = GEGL_IMAGE_IMPL(gegl_op_get_op_impl(GEGL_OP(image_input)));
      GeglRect preimage_rect;
      GeglTile *input_tile;

      /* Compute the ith inputs preimage. */
      if(GEGL_IS_IMAGE(op))
        gegl_image_compute_preimage(GEGL_IMAGE(op), &preimage_rect, &result_rect, i);
      else
        gegl_image_get_result_rect(image_input, &preimage_rect); 

      LOG_DEBUG("compute_op", 
                "Fetching input tile for %dth input", 
                i);

      /* Fetch the input tile. */
      input_tile = gegl_tile_mgr_fetch_input_tile(tile_mgr, 
                                                  input_image_impl, 
                                                  &preimage_rect);

      g_object_unref(input_image_impl);

      if(!input_tile)
        LOG_DEBUG("compute_op", 
                  "Couldnt find %d th input_tile", 
                  i);

      set_request(self, &requests[i+first_source_index], &preimage_rect, input_tile);

      /* Put the request on the list of requests */
      request_list = g_list_append(request_list, (gpointer)&requests[i+first_source_index]);
    }

  LOG_DEBUG("compute_op", 
            "%s", 
            "Calling prepare"); 

  gegl_op_impl_prepare(op_impl, request_list);

  LOG_DEBUG("compute_op", 
            "%s", 
            "Calling process"); 

  gegl_op_impl_process(op_impl, request_list);

  LOG_DEBUG("compute_op", 
            "%s", 
            "Calling finish"); 

  gegl_op_impl_finish(op_impl, request_list);

  /* Release all inputs. */
  gegl_tile_mgr_release_tiles(tile_mgr, request_list);

  /* Free request array. */
  g_free(requests);

  /* Free the lists  */ 
  g_list_free(request_list);
  if(image_inputs)
    g_list_free(image_inputs);

  g_object_unref(op_impl);
}

/**
 * set_request:
 * @self: a #GeglSimpleImageMgr
 * @request: the #GeglOpRequest to set. 
 * @rect: a rect.
 * @tile: a #GeglTile.
 *
 * Put @rect and @tile into @request. 
 *
 **/
static void 
set_request (GeglSimpleImageMgr * self, 
             GeglOpRequest * request, 
             GeglRect * rect, 
             GeglTile * tile)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_SIMPLE_IMAGE_MGR (self));
                
  gegl_rect_copy(&request->rect,rect);
  request->tile = tile;
}

GeglTile *
gegl_simple_image_mgr_get_tile (GeglSimpleImageMgr * self, 
                                    GeglImageImpl * image_impl)
{
  GeglTile * tile = image_impl->tile;
  return tile; 
}

static void 
add_image (GeglImageMgr * self_image_mgr, 
           GeglImageImpl * image_impl)
{
  GeglSimpleImageMgr *self = GEGL_SIMPLE_IMAGE_MGR(self_image_mgr);

  /* For an SampledImage, make sure the Tile and data are allocated. */
  if(GEGL_IS_SAMPLED_IMAGE_IMPL(image_impl))
    {
      GeglSampledImageImpl * sampled_image_impl = GEGL_SAMPLED_IMAGE_IMPL(image_impl);
      GeglTile * tile = NULL;
      gint width = gegl_sampled_image_impl_get_width(sampled_image_impl);
      gint height = gegl_sampled_image_impl_get_height(sampled_image_impl);
      GeglRect area;
      gegl_rect_set(&area,0,0,width,height);

      LOG_DEBUG("add_image", 
                "Making tile for %x", 
                (guint)image_impl);

      /* Allocate a tile for this image_impl. */
      tile = gegl_tile_mgr_make_tile(GEGL_IMAGE_MGR(self)->tile_mgr, image_impl, &area); 
      
      /* Make sure data is allocated. */
      LOG_DEBUG("add_image",
                "allocating tile(%x) for SampledImageImpl", 
                (guint)tile);

      gegl_tile_validate_data(tile);
    }
}
