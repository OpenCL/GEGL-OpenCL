#include "gegl-eval-visitor.h"
#include "gegl-filter.h"
#include "gegl-attributes.h"
#include "gegl-graph.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-color-model.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-tile.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglEvalVisitorClass * klass);
static void init (GeglEvalVisitor * self, GeglEvalVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static void validate_inputs(GeglEvalVisitor *self, GeglFilter * filter);
static void validate_outputs(GeglEvalVisitor *self, GeglFilter * filter);

static void validate_op_outputs(GeglEvalVisitor *self, GeglFilter * filter);
static void validate_image_outputs(GeglEvalVisitor *self, GeglFilter * filter);

static gpointer parent_class = NULL;

GType
gegl_eval_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_DFS_VISITOR, 
                                     "GeglEvalVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglEvalVisitor * self, 
      GeglEvalVisitorClass * klass)
{
}

static void
finalize(GObject *gobject)
{  
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GeglEvalVisitor *self = GEGL_EVAL_VISITOR(visitor); 
  GList * input_attributes;
  GList * attributes;

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  validate_inputs(self, filter); 

  input_attributes = 
    gegl_visitor_get_input_attributes(visitor, GEGL_NODE(filter));

  validate_outputs(self,filter);
  attributes = gegl_op_get_attributes(GEGL_OP(filter));

  gegl_dump_graph(GEGL_NODE(filter));
  LOG_DEBUG("visit_filter", 
            "calling evaluate of %s %p", 
            G_OBJECT_TYPE_NAME(filter), filter); 

  gegl_filter_evaluate(filter, 
                       attributes, 
                       input_attributes);

  g_list_free(attributes);
  g_list_free(input_attributes);
}

static void      
visit_graph(GeglVisitor * visitor,
             GeglGraph *graph)
{
  GeglGraph *prev_graph = visitor->graph;
  GEGL_VISITOR_CLASS(parent_class)->visit_graph(visitor, graph);

  visitor->graph = graph;
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(visitor), 
                            GEGL_NODE(graph->root));
  visitor->graph = prev_graph; 
}

void
validate_inputs(GeglEvalVisitor *self,
                GeglFilter * filter)
{
  GeglVisitor *visitor = GEGL_VISITOR(self); 
  gint i;
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(filter)); 
  GList *inputs_attributes_list = 
    gegl_visitor_get_input_attributes(visitor, GEGL_NODE(filter));

  for(i = 0; i < num_inputs; i++) 
    {
      GeglAttributes * input_attributes = 
        (GeglAttributes*)g_list_nth_data(inputs_attributes_list, i); 

      GeglTile *tile = (GeglTile*)g_value_get_object(input_attributes->value);

      /* Make sure the data is validated */
      if(tile)
        {
          GeglTileMgr *tile_mgr = gegl_tile_mgr_instance();
          LOG_DEBUG("validate_input_values", 
                    "validating tile %p for %dth input of %s %p", 
                    tile, i, G_OBJECT_TYPE_NAME(filter), filter);
          gegl_tile_mgr_validate_data(tile_mgr, tile);
          g_object_unref(tile_mgr);
        }
      else 
        {
          LOG_DEBUG("validate_input_values", 
                    "couldnt validate input %d image data for %s %p", 
                    i, G_OBJECT_TYPE_NAME(filter), filter);
        }
    }

  g_list_free(inputs_attributes_list);
}

static void 
validate_outputs(GeglEvalVisitor *self,
                 GeglFilter * filter) 
{
  if(GEGL_IS_IMAGE(filter))
    {
      validate_image_outputs(self, filter);
    }
  else if(GEGL_IS_OP(filter))
    {
      validate_op_outputs(self, filter);
    }
  else  
    {
      g_print("cant validate outputs\n");
    }
}

static void
validate_op_outputs(GeglEvalVisitor *self,
                    GeglFilter * filter) 
{
  GeglVisitor *visitor = GEGL_VISITOR(self); 
  GList * input_attributes_list = 
    gegl_visitor_get_input_attributes(visitor, GEGL_NODE(filter));

  GeglAttributes *attributes = 
    gegl_op_get_nth_attributes(GEGL_OP(filter), 0);

  GeglAttributes *input_attributes =(GeglAttributes*) 
    g_list_nth_data(input_attributes_list, 0); 

  /* Just copy input to output directly */
  g_value_copy(input_attributes->value, attributes->value);
}

static void
validate_image_outputs(GeglEvalVisitor *self,
                       GeglFilter * filter) 
{
  LOG_DEBUG("validate_image_outputs", 
            "validating output value for %s %p", 
            G_OBJECT_TYPE_NAME(filter), filter);
   {
      GeglAttributes * attributes = 
        gegl_op_get_nth_attributes(GEGL_OP(filter), 0); 
      GeglTile *tile = (GeglTile*)g_value_get_object(attributes->value);
      GeglTileMgr *tile_mgr = gegl_tile_mgr_instance();

      LOG_DEBUG("validate_outputs", 
                "output value is %p", attributes->value);

      if(!tile) 
        {
          GeglTile * image_tile = GEGL_IMAGE(filter)->tile;
          GeglColorModel *color_model = 
            gegl_attributes_get_color_model(attributes);

          /* Check the image tile, see if its there */
          LOG_DEBUG("validate_outputs", 
                    "no output tile using own tile %p for %s %p", 
                    image_tile, G_OBJECT_TYPE_NAME(filter), filter);

          /* Create the image tile if necessary */
          if(!image_tile)
            {
              gegl_image_set_color_model(GEGL_IMAGE(filter) , color_model);
              LOG_DEBUG("validate_outputs", 
                        "creating output tile for %s %p", 
                        G_OBJECT_TYPE_NAME(filter), filter);
              image_tile = gegl_tile_mgr_create_tile(tile_mgr, 
                                                     color_model, 
                                                     &attributes->rect); 
              gegl_tile_mgr_validate_data (tile_mgr,image_tile); 
            }
          else 
            {
              /* Validate the image tile. */
              image_tile = gegl_tile_mgr_validate_tile(tile_mgr, 
                                                       image_tile, 
                                                       &attributes->rect, 
                                                       color_model);
            }

          LOG_DEBUG("validate_outputs", 
                    "validated tile is %p for use by %s %p", 
                    image_tile, G_OBJECT_TYPE_NAME(filter), filter);

          /* Store the tile for this image op, and put in output value. */
          GEGL_IMAGE(filter)->tile = image_tile;

          LOG_DEBUG("validate_outputs", 
                    "the image tile in filter is %p", image_tile);

          g_value_set_tile(attributes->value, image_tile);
        }
      else 
        {
          LOG_DEBUG("validate_output_values", 
                    "found output tile %p for use by %s %p", 
                    tile, G_OBJECT_TYPE_NAME(filter), filter);
            
          /* Validate the data. */
          gegl_tile_mgr_validate_data (tile_mgr,tile); 
                                             
          LOG_DEBUG("validate_output_values", 
                    "validated data for tile %p for use by %s %p", 
                    tile, G_OBJECT_TYPE_NAME(filter), filter);

          /* Put in output value. */
          g_value_set_tile(attributes->value, tile);
        }

        g_object_unref(tile_mgr);
    }
}
