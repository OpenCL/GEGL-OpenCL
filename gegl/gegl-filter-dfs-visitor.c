#include "gegl-filter-dfs-visitor.h"
#include "gegl-op.h"
#include "gegl-filter.h"
#include "gegl-value-types.h"
#include "gegl-image.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglFilterDfsVisitorClass * klass);
static void init (GeglFilterDfsVisitor * self, GeglFilterDfsVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_op (GeglVisitor *visitor, GeglOp * op);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static GList * get_input_values(GeglFilterDfsVisitor * self, GeglOp *op);
static void validate_input_values(GeglFilterDfsVisitor *self, GeglOp * op, GList * input_values);
static void validate_output_values(GeglFilterDfsVisitor *self, GeglOp * op, GList * input_values);

static gpointer parent_class = NULL;

GType
gegl_filter_dfs_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterDfsVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilterDfsVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglFilterDfsVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFilterDfsVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  visitor_class->visit_op = visit_op;
  visitor_class->visit_filter = visit_filter;
}

static void 
init (GeglFilterDfsVisitor * self, 
      GeglFilterDfsVisitorClass * klass)
{
  self->input_values = NULL;
  self->filter = NULL;
}

static void
finalize(GObject *gobject)
{  
  GeglFilterDfsVisitor * self = GEGL_FILTER_DFS_VISITOR(gobject);

  if(self->input_values)
    g_list_free(self->input_values);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void      
visit_op(GeglVisitor * visitor,
         GeglOp *op)
{
  GeglFilterDfsVisitor *self = GEGL_FILTER_DFS_VISITOR(visitor); 
  GList * input_values;
  GList * output_values;

  GEGL_VISITOR_CLASS(parent_class)->visit_op(visitor, op);

  input_values = get_input_values(self, op);
  output_values = gegl_op_get_output_values(op);

  LOG_DEBUG("visit_op", 
            "computing have rect for %s %x", 
            G_OBJECT_TYPE_NAME(op),(guint)op);
  gegl_op_compute_have_rect(op, input_values);

  LOG_DEBUG("visit_op", 
             "computing derived color_model for %s %x", 
             G_OBJECT_TYPE_NAME(op), (guint)op);
  gegl_op_compute_derived_color_model(op, input_values);

  validate_input_values(self, op, input_values); 
  validate_output_values(self, op, output_values); 

  gegl_op_evaluate(op, output_values, input_values);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  GeglFilterDfsVisitor *self = GEGL_FILTER_DFS_VISITOR(visitor); 
  GValue *filter_output_value;
  GValue *root_output_value;
  GeglFilterDfsVisitor * filter_dfs_visitor; 

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(visitor, filter);

  filter_dfs_visitor= g_object_new(GEGL_TYPE_FILTER_DFS_VISITOR, NULL); 
  filter_dfs_visitor->input_values = get_input_values(self, GEGL_OP(filter));
  filter_dfs_visitor->filter = filter;

  LOG_DEBUG("visit_filter", 
            "creating dfs visitor for filter %x",
             (guint)filter);

  gegl_node_traverse_depth_first(GEGL_NODE(filter->root), 
                                 GEGL_VISITOR(filter_dfs_visitor),
                                 TRUE);

  g_object_unref(filter_dfs_visitor);

  filter_output_value = gegl_op_get_nth_output_value(GEGL_OP(filter), 0);
  root_output_value = gegl_op_get_nth_output_value(filter->root, 0);

  LOG_DEBUG("visit_filter", 
            "setting filter have rect from root have rect");

  g_value_copy(root_output_value, filter_output_value);

  g_value_print_image_data(root_output_value, "root");
  g_value_print_image_data(filter_output_value, "filter");
}

static GList *
get_input_values(GeglFilterDfsVisitor * self,
                 GeglOp *op)
{
  GList * input_values;

  if(self->filter)
    input_values = gegl_filter_get_input_values(self->filter, op);
  else
    input_values = NULL;

  return input_values;
}

static void
validate_input_values(GeglFilterDfsVisitor *self,
                      GeglOp * op,
                      GList * input_values)
{
  gint i;
  gint num_inputs = g_list_length(input_values);

  for(i = 0; i < num_inputs; i++) 
    {
      GValue *input_value = g_list_nth_data(input_values, i);

      if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
        {
          GeglRect rect;
          GeglTile *tile = g_value_get_image_data_tile(input_value);
          g_value_get_image_data_rect(input_value, &rect);

          /* Make sure the data is validated */
          if(tile)
            {
              GeglTileMgr *tile_mgr = gegl_tile_mgr_instance();
              LOG_DEBUG("validate_input_values", 
                        "validating tile %x for %dth input of %s %x", 
                        tile, i, G_OBJECT_TYPE_NAME(op), (guint)op);
              gegl_tile_mgr_validate_data(tile_mgr, tile);
              g_object_unref(tile_mgr);
            }
          else 
            {
              LOG_DEBUG("validate_input_values", 
                        "couldnt validate input %d image data for %s %x", 
                        i, G_OBJECT_TYPE_NAME(op), (guint)op);
            }
        }
      else 
        {
          g_warning("input value isnt image data");
        }
    }
}

void
validate_output_values(GeglFilterDfsVisitor *self,
                       GeglOp * op,
                       GList * output_values)
{
  gint i;
  gint num_outputs = g_list_length(output_values);

  for(i = 0; i < num_outputs; i++) 
    {
      GValue *output_value = g_list_nth_data(output_values, i);

      if(G_VALUE_HOLDS_IMAGE_DATA(output_value))
        {
          GeglRect rect;
          g_value_get_image_data_rect(output_value, &rect);

          if(GEGL_IS_IMAGE(op)) 
            {
              GeglTileMgr *tile_mgr = gegl_tile_mgr_instance();
              GeglTile *output_tile = g_value_get_image_data_tile(output_value);

              if(!output_tile) 
                {
                  LOG_DEBUG("validate_output_values", 
                            "no output tile using own tile for %s %x", 
                            G_OBJECT_TYPE_NAME(op), (guint)op);

                  /* Make sure the tile is created */
                  if(!GEGL_IMAGE(op)->tile)
                    {
                      GeglColorModel *color_model = gegl_image_color_model(GEGL_IMAGE(op));
                      LOG_DEBUG("validate_output_values", 
                                "creating own tile for %s %x", 
                                G_OBJECT_TYPE_NAME(op), (guint)op);
                      GEGL_IMAGE(op)->tile = 
                        gegl_tile_mgr_create_tile(tile_mgr, color_model, &rect); 
                    }

                  /* Set it to the tile of op and the output_value */

                  GEGL_IMAGE(op)->tile = 
                    gegl_tile_mgr_validate_tile(tile_mgr, 
                                                GEGL_IMAGE(op)->tile, &rect);


                  /* Make sure the tile is valid (also validate data) */
                  LOG_DEBUG("validate_output_values", 
                            "validated tile %x for use by %s %x", 
                            GEGL_IMAGE(op)->tile, G_OBJECT_TYPE_NAME(op), (guint)op);

                  g_value_set_image_data_tile(output_value, GEGL_IMAGE(op)->tile);
                }
              else 
                {
                  LOG_DEBUG("validate_output_values", 
                            "found output tile %x for use by %s %x", 
                            output_tile, G_OBJECT_TYPE_NAME(op), (guint)op);
                    
                  /* Make sure the tile is valid (also validate data) */
                  output_tile = gegl_tile_mgr_validate_tile(tile_mgr, 
                                                            output_tile, &rect);

                  LOG_DEBUG("validate_output_values", 
                            "validated tile %x for use by %s %x", 
                            output_tile, G_OBJECT_TYPE_NAME(op), (guint)op);

                  g_value_set_image_data_tile(output_value, output_tile);
                }


                g_object_unref(tile_mgr);

            }
          else
            {
              LOG_DEBUG("validate_output_value", 
                        "couldnt validate output %d image data for %s %x", 
                        i, G_OBJECT_TYPE_NAME(op), (guint)op);
            }
        }
      else 
        {
          g_warning("output value isnt image data");
        }
    }
}
