#include "gegl-filter.h"
#include "gegl-node.h"
#include "gegl-image.h"
#include "gegl-visitor.h"
#include "gegl-bfs-visitor.h"
#include "gegl-dfs-visitor.h"
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

static void apply (GeglOp*op);
static void accept (GeglNode * node, GeglVisitor * visitor);

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
  GeglNodeClass *node_class = GEGL_NODE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  node_class->accept = accept;
  op_class->apply = apply;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this filter",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        "image",
                                                        "A destination image for result",
                                                         GEGL_TYPE_OP,
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
  GeglOp *op = GEGL_OP(self);
  GValue * output_value;

  gegl_op_set_num_output_values(op, 1);
  output_value = gegl_op_get_nth_output_value(op,0);
  g_value_init(output_value, GEGL_TYPE_IMAGE_DATA);

  gegl_rect_set(&self->roi,0,0,GEGL_DEFAULT_WIDTH,GEGL_DEFAULT_HEIGHT);
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

GList *
gegl_filter_get_input_values(GeglFilter *self, 
                             GeglOp *op)
{
  gint i;
  GeglInputInfo * input_infos = gegl_node_get_input_infos(GEGL_NODE(op));
  GList * input_values = NULL;
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(op));
  for(i = 0; i < num_inputs; i++) 
    {
      GValue *input_value = NULL;

      if(input_infos[i].input)
        input_value = gegl_op_get_nth_output_value(GEGL_OP(input_infos[i].input),
                                                   input_infos[i].index); 

      input_values = g_list_append(input_values, input_value); 
    }

  g_free(input_infos);
  return input_values;
}

static void      
apply (GeglOp * op)
{
  GeglFilter *self = GEGL_FILTER(op);
  GValue *output_value = gegl_op_get_nth_output_value(op, 0);
  g_value_set_image_data_rect(output_value, &self->roi);

  if(self->image)
    {
     LOG_DEBUG("apply", "setting tile for passed dest on filter");
     g_value_set_image_data_tile(output_value, GEGL_IMAGE(self->image)->tile);
    }

  LOG_DEBUG("apply", "visit breadth first");
  {
    GeglVisitor * bfs_visitor = g_object_new(GEGL_TYPE_BFS_VISITOR, NULL); 
    gegl_visitor_visit_filter(bfs_visitor, self);
    g_object_unref(bfs_visitor);
  }

  LOG_DEBUG("apply", "visit depth first");
  {
    GeglVisitor * dfs_visitor = g_object_new(GEGL_TYPE_DFS_VISITOR, NULL); 
    gegl_visitor_visit_filter(dfs_visitor, self);
    g_object_unref(dfs_visitor);
  }
}

static void              
accept (GeglNode * node, 
        GeglVisitor * visitor)
{
  gegl_visitor_visit_filter(visitor, GEGL_FILTER(node));
}
