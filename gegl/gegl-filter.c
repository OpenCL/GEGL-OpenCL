#include "gegl-filter.h"
#include "gegl-node.h"
#include "gegl-image.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_ROOT, 
  PROP_IMAGE, 
  PROP_ROI, 
  PROP_LAST 
};

static void class_init (GeglFilterClass * klass);
static void init (GeglFilter * self, GeglFilterClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean evaluate_breadth_first (GeglNode * node, gpointer data);
static gboolean evaluate_depth_first (GeglNode * node, gpointer data);
static void apply(GeglOp*op);
static void initialize_input_values(GeglFilter * self, GList * input_values);

static GList * get_input_values(GeglFilter *self, GeglOp * op);
static GList * get_output_values(GeglFilter *self, GeglOp * op);
static void validate_input_values(GeglFilter *self, GeglOp * op, GList *input_values);
static void validate_output_values(GeglFilter *self, GeglOp * op, GList *output_values);

static gpointer parent_class = NULL;

GType
gegl_filter_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglFilterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglFilter),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OP , 
                                     "GeglFilter", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglOpClass *op_class = GEGL_OP_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  op_class->compute_need_rect = NULL;
  op_class->compute_have_rect = NULL;
  op_class->apply = apply;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this filter",
                                                         GEGL_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        "image",
                                                        "A destination image for result",
                                                         GEGL_TYPE_OBJECT,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ROI,
                                   g_param_spec_pointer ("roi",
                                                        "roi",
                                                        "Region of interest for filter",
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglFilter * self, 
      GeglFilterClass * klass)
{
  /*
  gegl_rect_set(&self->roi,-G_MAXINT/2,-G_MAXINT/2,G_MAXINT,G_MAXINT);
  */

  gegl_rect_set(&self->roi,0,0,720,486);
  self->image = NULL;
  self->root = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglFilter * filter = GEGL_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
        gegl_filter_set_root(filter, (GeglOp*)g_value_get_object(value));  
      break;
    case PROP_IMAGE:
        gegl_filter_set_image(filter, (GeglSampledImage*)g_value_get_object(value));  
      break;
    case PROP_ROI:
        gegl_filter_set_roi (filter, (GeglRect*)g_value_get_pointer (value));
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
  GeglFilter * filter = GEGL_FILTER(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
        g_value_set_object(value, (GObject*)gegl_filter_get_root(filter));  
      break;
    case PROP_IMAGE:
        g_value_set_object(value, (GObject*)gegl_filter_get_image(filter));  
      break;
    case PROP_ROI:
      gegl_filter_get_roi (filter, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      break;
  }
}

GeglOp* 
gegl_filter_get_root (GeglFilter * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILTER (self), NULL);

  return self->root;
}

void 
gegl_filter_set_root (GeglFilter * self,
                      GeglOp *root)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));

  self->root = root;
}

void
gegl_filter_get_roi(GeglFilter *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));

  gegl_rect_copy(roi, &self->roi);
}

void
gegl_filter_set_roi(GeglFilter *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));

  if(roi)
    gegl_rect_copy(&self->roi, roi);
  else
    {
      if(self->image && GEGL_IS_SAMPLED_IMAGE(self->image))
        {
          gint width = gegl_sampled_image_get_width(self->image);
          gint height = gegl_sampled_image_get_height(self->image);
          gegl_rect_set(&self->roi,0,0,width,height);
        }
      else
        {
            gegl_rect_set(&self->roi,0,0,720,486);
        }
    }
}

GeglSampledImage* 
gegl_filter_get_image (GeglFilter * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_FILTER (self), NULL);

  return self->image;
}

void 
gegl_filter_set_image (GeglFilter * self,
                       GeglSampledImage *image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_FILTER (self));

  self->image = image;
}

static GList *
get_input_values(GeglFilter *self,
                 GeglOp * op)
{
  gint i;
  GeglInputInfo * input_infos = gegl_node_get_input_infos(GEGL_NODE(op));
  GList * input_values = NULL;
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(op));
  for(i = 0; i < num_inputs; i++) 
    {
      GValue *input_value = NULL;

      if(input_infos[i].input)
        input_value = GEGL_OP(input_infos[i].input)->output_value; 

      input_values = g_list_append(input_values, input_value); 
    }

  return input_values;
}

static void
validate_input_values(GeglFilter *self,
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
              LOG_DEBUG("gegl_filter_validate_input_value", 
                        "validating tile %x for %dth input of %s %x", 
                        tile, i, G_OBJECT_TYPE_NAME(op), (guint)op);
              gegl_tile_mgr_validate_data(tile_mgr, tile);
              g_object_unref(tile_mgr);
            }
          else 
            {
              LOG_DEBUG("gegl_filter_validate_input_value", 
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

static GList *
get_output_values(GeglFilter *self,
                  GeglOp * op)
{
  GValue * output_value = gegl_op_get_output_value(op);
  return g_list_append(NULL, output_value);
}

static void
validate_output_values(GeglFilter *self,
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
                  LOG_DEBUG("gegl_filter_validate_output_value", 
                            "no output tile using own tile for %s %x", 
                            G_OBJECT_TYPE_NAME(op), (guint)op);

                  /* Make sure the tile is created */
                  if(!GEGL_IMAGE(op)->tile)
                    {
                      GeglColorModel *color_model = gegl_image_color_model(GEGL_IMAGE(op));
                      LOG_DEBUG("gegl_filter_validate_output_value", 
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
                  LOG_DEBUG("gegl_filter_validate_output_value", 
                            "validated tile %x for use by %s %x", 
                            GEGL_IMAGE(op)->tile, G_OBJECT_TYPE_NAME(op), (guint)op);

                  g_value_set_image_data_tile(output_value, GEGL_IMAGE(op)->tile);
                }
              else 
                {
                  LOG_DEBUG("gegl_filter_validate_output_value", 
                            "found output tile %x for use by %s %x", 
                            output_tile, G_OBJECT_TYPE_NAME(op), (guint)op);
                    
                  /* Make sure the tile is valid (also validate data) */
                  output_tile = gegl_tile_mgr_validate_tile(tile_mgr, 
                                                            output_tile, &rect);

                  LOG_DEBUG("gegl_filter_validate_output_value", 
                            "validated tile %x for use by %s %x", 
                            output_tile, G_OBJECT_TYPE_NAME(op), (guint)op);

                  g_value_set_image_data_tile(output_value, output_tile);
                }


                g_object_unref(tile_mgr);

            }
          else
            {
              LOG_DEBUG("gegl_filter_validate_output_value", 
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
                        
static gboolean 
evaluate_breadth_first(GeglNode *node, 
                       gpointer data)
{
  if(GEGL_IS_FILTER(node))
    {
      GeglFilter *filter = GEGL_FILTER(node);
      GValue *root_output_value = filter->root->output_value;
      GValue *filter_output_value = GEGL_OP(filter)->output_value;

      /* Set the need_rect of root from the filter need_rect.  */
      /* LOG_DEBUG("evaluate_breadth_first", 
                   "setting root need rect from filter"); */ 
      g_value_copy(filter_output_value, root_output_value);

      /* LOG_DEBUG("evaluate_breadth_first", 
                   "after setting root need rect from filter"); */ 

      g_value_print_image_data(filter_output_value, "filter");
      g_value_print_image_data(root_output_value, "root");

      /* Traverse the graph described by filter */
      gegl_node_traverse_breadth_first(GEGL_NODE(filter->root), 
                                       evaluate_breadth_first, 
                                       filter);
    }
  else 
    {
      GeglFilter * filter = (GeglFilter*)data; 
      GList * input_values = get_input_values(filter, GEGL_OP(node));

      initialize_input_values(filter, input_values);

      /* Set the need rects for the inputs of this node */ 
      gegl_op_compute_need_rects(GEGL_OP(node), input_values);
      g_list_free(input_values);
    }

  return FALSE;
}

static void
initialize_input_values(GeglFilter * self,
                        GList * input_values)
{
  gint i;
  gint num_inputs = g_list_length(input_values);

  for(i = 0; i < num_inputs; i++) 
    {
      GValue *input_value = g_list_nth_data(input_values, i);

      if(G_VALUE_HOLDS_IMAGE_DATA(input_value))
          g_value_set_image_data_tile(input_value, NULL);
      else 
          g_warning("input value isnt image data");
    }
}
    


static gboolean 
evaluate_depth_first(GeglNode *node, 
                     gpointer data)
{
  if(GEGL_IS_FILTER(node))
    {
      GeglFilter *filter = GEGL_FILTER(node);
      GValue *root_output_value;
      GValue *filter_output_value;

      /* Traverse the graph described by filter */
      gegl_node_traverse_depth_first(GEGL_NODE(filter->root), 
                                     evaluate_depth_first, 
                                     filter, 
                                     TRUE);

      root_output_value = filter->root->output_value;
      filter_output_value = GEGL_OP(filter)->output_value;
      
      /* Set the have_rect of filter from the root have_rect.  */
      g_value_copy(root_output_value, filter_output_value);
    }
  else
    {
      GeglFilter * filter = (GeglFilter*)data; 
      GList * output_values = get_output_values(filter, GEGL_OP(node));
      GList * input_values = get_input_values(filter, GEGL_OP(node));

      validate_input_values(filter, GEGL_OP(node), input_values);

      /* Compute the have_rect for this node */ 
      gegl_op_compute_have_rect(GEGL_OP(node), input_values);

      /* Compute the derived color model for this node */ 
      if(GEGL_IS_IMAGE(node))
        gegl_image_compute_derived_color_model(GEGL_IMAGE(node), input_values);

      validate_output_values(filter, GEGL_OP(node), output_values);

      /* Evaluate the node */ 
      gegl_op_evaluate(GEGL_OP(node), output_values, input_values);

      g_list_free(input_values);
      g_list_free(output_values);
    }

  return FALSE;
}

static void      
apply (GeglOp * op)
{
  GeglFilter *self = GEGL_FILTER(op);

  g_value_set_image_data_rect(GEGL_OP(self)->output_value, 
                              &self->roi);

  if(self->image)
    {
     LOG_DEBUG("apply", "setting tile for passed dest on filter");
     g_value_set_image_data_tile(GEGL_OP(self)->output_value, 
                                GEGL_IMAGE(self->image)->tile);
    }

  /* Traverse breadth first */
  evaluate_breadth_first(GEGL_NODE(self), NULL);

  /* Traverse depth first */
  evaluate_depth_first(GEGL_NODE(self), NULL);
}
