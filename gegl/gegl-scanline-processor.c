#include "gegl-scanline-processor.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-image-iterator.h"
#include "gegl-data.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglScanlineProcessorClass * klass);
static void init (GeglScanlineProcessor * self, GeglScanlineProcessorClass * klass);
static void finalize(GObject *gobject);
static void get_iterator(GeglScanlineProcessor *self, GeglData *data);
static void advance_iterator (gpointer key, gpointer value, gpointer user_data);
static void free_iterator (gpointer key, gpointer value, gpointer user_data);

static gpointer parent_class = NULL;

GType
gegl_scanline_processor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglScanlineProcessorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglScanlineProcessor),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglScanlineProcessor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglScanlineProcessorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);
  gobject_class->finalize = finalize;
}

static void 
init (GeglScanlineProcessor * self, 
      GeglScanlineProcessorClass * klass)
{
  self->iterators = g_hash_table_new(g_str_hash,g_str_equal);
}

static void
finalize(GObject *gobject)
{
  GeglScanlineProcessor *self = GEGL_SCANLINE_PROCESSOR(gobject);

  g_hash_table_destroy(self->iterators);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

GeglImageIterator *
gegl_scanline_processor_lookup_iterator(GeglScanlineProcessor *self,
                                        const gchar *name)
{
  return (GeglImageIterator*)g_hash_table_lookup(self->iterators, name); 
}

static void
advance_iterator (gpointer key,
                  gpointer value,
                  gpointer user_data)
{
  GeglImageIterator *iter = (GeglImageIterator*)value;
  gegl_image_iterator_next(iter);
}

static void
free_iterator (gpointer key,
               gpointer value,
               gpointer user_data)
{
  GeglImageIterator *iter = (GeglImageIterator*)value;
  g_object_unref (iter); 
}

static void
get_iterator(GeglScanlineProcessor *self,
             GeglData *data)
{
  if(GEGL_IS_IMAGE_DATA(data))
    {
      GValue *value = gegl_data_get_value(data);
      GeglImageData *image_data = GEGL_IMAGE_DATA(data);
      GeglImage *image = (GeglImage*)g_value_get_object(value);
      GeglRect rect;
      GeglImageIterator *iterator;

      gegl_image_data_get_rect(image_data, &rect);
      iterator = g_object_new (GEGL_TYPE_IMAGE_ITERATOR, 
                                "image", image,
                                "area", &rect,
                                NULL);  
      gegl_image_iterator_first (iterator);
      g_hash_table_insert(self->iterators, 
                          (gpointer)gegl_data_get_name(GEGL_DATA(data)), 
                          iterator);
    }
}
                     

/**
 * gegl_scanline_processor_process:
 * @self: a #GeglScanlineProcessor.
 *
 * Process scanlines.
 *
 **/
void 
gegl_scanline_processor_process (GeglScanlineProcessor * self)
{
  gint i;
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(self->op));
  gint num_outputs = gegl_node_get_num_outputs(GEGL_NODE(self->op));

#if 1 
  gegl_log_debug("processor_process", 
                 "%s %p", G_OBJECT_TYPE_NAME(self->op), self->op); 

  gegl_log_debug("processor_process", 
                 "inputs %d outputs %d", num_inputs, num_outputs); 
#endif

  /* Get iterator for each image output */
  for(i=0; i < num_outputs; i++)
    {
      GeglData *data = gegl_op_get_nth_output_data(GEGL_OP(self->op), i);
      get_iterator(self, data);
    }

  /* Get iterator for each image input */
  for(i=0; i < num_inputs; i++)
    {
      GeglData *data = gegl_op_get_nth_input_data(GEGL_OP(self->op), i);
      get_iterator(self, data);
    }

  /* Get the rect we want to process */
  {
    GeglData *data = gegl_op_get_nth_output_data(GEGL_OP(self->op), 0);
    GeglImageData *image_data = GEGL_IMAGE_DATA(data);
    GeglRect rect;
    gegl_image_data_get_rect(image_data, &rect);

    gegl_log_debug("processor_process", 
                   "width height %d %d", rect.w, rect.h);

    /* Iterate over the scanlines */
    for(i=0; i < rect.h; i++)
      {
        /*
        gegl_log_debug("processor_process", 
                       "doing scanline %d", i);
        */

        /* Call the subclass scanline func. */
        (self->func)(self->op, self, rect.w);

        /* Advance all the iterators to next scanline. */
        g_hash_table_foreach(self->iterators, advance_iterator, NULL);
      } 
  }

  /* Free all the iterators */
  g_hash_table_foreach(self->iterators, free_iterator, NULL);
}
