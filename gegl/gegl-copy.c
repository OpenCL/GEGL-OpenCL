#include "gegl-copy.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

enum
{
  PROP_0, 
  PROP_INPUT,
  PROP_LAST 
};

static void class_init (GeglCopyClass * klass);
static void init (GeglCopy * self, GeglCopyClass * klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void finalize (GObject *gobject);

static void allocate_xyz_data (GeglCopy * self, gint width);
static void free_xyz_data (GeglCopy * self);

static void prepare (GeglFilter * op, GList * attributes, GList *input_attributes);
static void scanline (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static GeglColorModel *compute_derived_color_model (GeglFilter * filter, GList* input_color_models);

static gpointer parent_class = NULL;

GType
gegl_copy_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCopyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCopy),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP , 
                                     "GeglCopy", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCopyClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  filter_class->prepare = prepare;
  filter_class->compute_derived_color_model = compute_derived_color_model;

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input of GeglCopy",
                                                         GEGL_TYPE_OP,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglCopy * self, 
      GeglCopyClass * klass)
{
  gint i;
  GeglNode * node = GEGL_NODE(self);

  self->convert_func = NULL;
  for(i = 0; i < 4; i++)
    self->float_xyz_data[i] = NULL;


  gegl_node_set_num_outputs(node, 1);
  gegl_node_set_num_inputs(node, 1);

  return;
}

static void
finalize(GObject *gobject)
{
   GeglCopy *self = GEGL_COPY (gobject);

   free_xyz_data (self);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUT:
      g_value_set_object(value, (GObject*)gegl_node_get_nth_input(node, 0));  
      break;
    default:
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (prop_id)
  {
    case PROP_INPUT:
      gegl_node_set_nth_input(node, (GeglNode*)g_value_get_object(value), 0);  
      break;
    default:
      break;
  }
}

/**
 * allocate_xyz_data:
 * @self: a #GeglCopy.
 * @width: scanline width.
 *
 * Allocate a scanline of xyz data.
 *
 **/
static void 
allocate_xyz_data (GeglCopy * self, 
                   gint width)
{
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COPY (self));

  for(i = 0; i < 4; i++)
    self->float_xyz_data[i] = 
  g_malloc (sizeof(gfloat) * width); 
}

/**
 * free_xyz_data:
 * @self: a #GeglCopy.
 *
 * Free the scanline of xyz data.
 *
 **/
static void 
free_xyz_data (GeglCopy * self)
{
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COPY (self));

  for(i = 0; i < 4; i++)
    {
      if (self->float_xyz_data[i]) 
        g_free (self->float_xyz_data[i]);

      self->float_xyz_data[i] = NULL; 
    }   
}

static GeglColorModel * 
compute_derived_color_model (GeglFilter * filter, 
                             GList * input_color_models)
{
  GeglColorModel *cm = gegl_image_color_model(GEGL_IMAGE(filter));
  if(cm) 
    return cm; 
  else 
    return (GeglColorModel *)g_list_nth_data(input_color_models, 0);
}

static void 
prepare (GeglFilter * filter, 
         GList *attributes,
         GList *input_attributes)
{
  GeglPointOp *point_op = GEGL_POINT_OP(filter); 
  GeglCopy *self = GEGL_COPY(filter); 

  GeglAttributes *dest_attributes = g_list_nth_data(attributes, 0); 
  GeglTile *dest = (GeglTile*)g_value_get_object(dest_attributes->value);
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest);
  GeglRect dest_rect;

  GeglAttributes *src_attributes = g_list_nth_data(input_attributes, 0); 
  GeglTile *src = (GeglTile*)g_value_get_object(src_attributes->value);
  GeglColorModel * src_cm = gegl_tile_get_color_model (src);

  gegl_rect_copy(&dest_rect, &dest_attributes->rect);

  g_return_if_fail(dest_cm);
  g_return_if_fail(src_cm);

  {
    /* Get the name of the interface that converts from src cm */
    gchar *converter_name = gegl_color_model_get_convert_interface_name (src_cm);

    /* Check to see if the dest cm implements the converter */
    self->convert_func =
      (GeglConvertFunc)gegl_object_query_interface (GEGL_OBJECT(dest_cm),
                                                    converter_name);

    g_free (converter_name);

    if (!self->convert_func)
      {
        /* Allocate scanlines for the XYZ data */
        free_xyz_data (self);
        allocate_xyz_data (self, dest_rect.w);
      }

    /* Set the scanline version of this class to be called */
    point_op->scanline_processor->func = scanline;
  }
}

/**
 * scanline:
 * @filter: a #GeglFilter.
 * @iters: a #GeglTileIterator array. 
 * @width: width of scanline.
 *
 * Process a scanline.
 *
 **/
static void 
scanline (GeglFilter * filter, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglCopy *self = GEGL_COPY (filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);
  guint dest_nchans = gegl_color_model_num_channels (dest_cm);
  guint src_nchans = gegl_color_model_num_channels (src_cm);
  guchar ** dest_data = g_malloc (sizeof(guchar*) * dest_nchans);
  guchar ** src_data = g_malloc (sizeof(guchar*) * src_nchans);

  gegl_tile_iterator_get_current(iters[0], (gpointer*)dest_data);
  gegl_tile_iterator_get_current(iters[1], (gpointer*)src_data);

  /* Call the converter if installed, else do XYZ conversion */ 
     
  if (self->convert_func) 
    (self->convert_func) (dest_cm, src_cm, dest_data, src_data, width); 
  else
    {
      gegl_color_model_convert_to_xyz (src_cm, 
                                       self->float_xyz_data, 
                                       src_data, 
                                       width);
      gegl_color_model_convert_from_xyz (dest_cm, 
                                         dest_data, 
                                         self->float_xyz_data , 
                                         width);
    }

  g_free (dest_data);
  g_free (src_data);
}
