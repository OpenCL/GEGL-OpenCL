#include "gegl-copy-impl.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

static void class_init (GeglCopyImplClass * klass);
static void init (GeglCopyImpl * self, GeglCopyImplClass * klass);
static void finalize (GObject *gobject);

static void allocate_xyz_data (GeglCopyImpl * self, gint width);
static void free_xyz_data (GeglCopyImpl * self);

static void prepare (GeglOpImpl * self_op_impl, GList * requests);
static void scanline (GeglOpImpl * self_op_impl, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_copy_impl_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCopyImplClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCopyImpl),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_POINT_OP_IMPL , 
                                     "GeglCopyImpl", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCopyImplClass * klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GeglOpImplClass *op_impl_class = GEGL_OP_IMPL_CLASS (klass);

   parent_class = g_type_class_peek_parent(klass);

   gobject_class->finalize = finalize;

   op_impl_class->prepare = prepare;

   return;
}

static void 
init (GeglCopyImpl * self, 
      GeglCopyImplClass * klass)
{
  gint i;
  GeglOpImpl * self_op_impl = GEGL_OP_IMPL(self);
  self_op_impl->num_inputs = 1;

  self->convert_func = NULL;
  for(i = 0; i < 4; i++)
    self->float_xyz_data[i] = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
   GeglCopyImpl *self = GEGL_COPY_IMPL (gobject);

   free_xyz_data (self);

   G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * allocate_xyz_data:
 * @self: a #GeglConvertImpl.
 * @width: scanline width.
 *
 * Allocate a scanline of xyz data.
 *
 **/
static void 
allocate_xyz_data (GeglCopyImpl * self, 
                   gint width)
{
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COPY_IMPL (self));

  for(i = 0; i < 4; i++)
    self->float_xyz_data[i] = 
  g_malloc (sizeof(gfloat) * width); 
}

/**
 * free_xyz_data:
 * @self: a #GeglConvertImpl.
 *
 * Free the scanline of xyz data.
 *
 **/
static void 
free_xyz_data (GeglCopyImpl * self)
{
  gint i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_COPY_IMPL (self));

  for(i = 0; i < 4; i++)
    {
      if (self->float_xyz_data[i]) 
        g_free (self->float_xyz_data[i]);

      self->float_xyz_data[i] = NULL; 
    }   
}

static void 
prepare (GeglOpImpl * self_op_impl, 
         GList * requests)
{
  GeglCopyImpl *self = GEGL_COPY_IMPL(self_op_impl);
  GeglPointOpImpl *self_point_op_impl = GEGL_POINT_OP_IMPL(self_op_impl);
  GeglOpRequest *dest_request = (GeglOpRequest*)g_list_nth_data(requests,0); 
  GeglColorModel * dest_cm = gegl_tile_get_color_model (dest_request->tile);
  GeglOpRequest *src_request = (GeglOpRequest*)g_list_nth_data(requests,1);
  GeglColorModel * src_cm = gegl_tile_get_color_model (src_request->tile);

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
        allocate_xyz_data (self, dest_request->rect.w);
      }

    /* Set the scanline version of this class to be called */
    self_point_op_impl->scanline_processor->func = scanline;
  }
}

/**
 * scanline:
 * @self_op_impl: a #GeglOpImpl.
 * @iters: a #GeglTileIterator array. 
 * @width: width of scanline.
 *
 * Process a scanline.
 *
 **/
static void 
scanline (GeglOpImpl * self_op_impl, 
          GeglTileIterator ** iters, 
          gint width)
{
  GeglCopyImpl *self = GEGL_COPY_IMPL (self_op_impl);
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
