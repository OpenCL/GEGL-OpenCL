#include "gegl-scanline-processor.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-image-iterator.h"
#include "gegl-data.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglScanlineProcessorClass * klass);
static gpointer parent_class = NULL;
static GList * get_image_data_list(GeglScanlineProcessor *self, GList *list);
static GList * get_input_image_data_list(GeglScanlineProcessor *self);
static GList * get_output_image_data_list(GeglScanlineProcessor *self);

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
        (GInstanceInitFunc) NULL,
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
  parent_class = g_type_class_peek_parent(klass);
  return;
}

static
GList *
get_output_image_data_list(GeglScanlineProcessor *self)
{
  GeglOp *op = GEGL_OP(self->op);
  gint num_outputs = gegl_node_get_num_outputs(GEGL_NODE(op));
  gint i;
  GList *list = NULL;

  for(i = 0; i < num_outputs; i++)
    { 
      GeglData * data = gegl_op_get_nth_output_data(op, i);

      if(GEGL_IS_IMAGE_DATA(data))
        list = g_list_append(list, data);
    }

  return list;
}

static
GList *
get_input_image_data_list(GeglScanlineProcessor *self)
{
  GeglOp *op = GEGL_OP(self->op);
  gint num_inputs = gegl_node_get_num_inputs(GEGL_NODE(op));
  gint i;
  GList *list = NULL;

  for(i = 0; i < num_inputs; i++)
    { 
      GeglData * data = gegl_op_get_nth_input_data(op, i);

      if(GEGL_IS_IMAGE_DATA(data))
        list = g_list_append(list, data);
    }

  return list;
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
  gint i,j;
  GeglRect rect;
  GeglImage *image;
  gint width, height;

  GList *image_output_data_list = get_output_image_data_list(self); 
  GList *image_input_data_list = get_input_image_data_list(self); 
  gint num_inputs = g_list_length(image_input_data_list);
  gint num_outputs = g_list_length(image_output_data_list);

  GeglData *data = g_list_nth_data(image_output_data_list,0);
  GValue *value = gegl_data_get_value(data);
  GeglImageData *image_data = GEGL_IMAGE_DATA(data);
  GeglImageIterator **iters = g_new (GeglImageIterator*, num_inputs + num_outputs);

#if 1 
  /*
  gegl_log_debug("processor_process", "%s %p", 
            G_OBJECT_TYPE_NAME(self->op), self->op); 
  */

  gegl_log_debug("processor_process", 
                 "%s %p", G_OBJECT_TYPE_NAME(self->op), self->op); 

  gegl_log_debug("processor_process", "inputs %d outputs %d", 
             num_inputs, num_outputs); 
#endif

  /* Get image iterators for outputs. */
  if (num_outputs == 1)
    {
      /*
       gegl_log_debug("processor_process", 
                 "getting image iterator for output %d", 
                 i);
      */

       image = (GeglImage*)g_value_get_object(value);
       gegl_image_data_get_rect(image_data, &rect);

       /* Get the image, if it is not NULL */ 
       if(image)
         {
           gegl_log_debug("processor_process", 
                     "output value image is %p", 
                     image);

           iters[0] = g_object_new (GEGL_TYPE_IMAGE_ITERATOR, 
                                    "image", image,
                                    "area", &rect,
                                    NULL);  

           gegl_image_iterator_first (iters[0]);
         }
    }

  /* Get image iterators for inputs. */
  for (i = 0; i < num_inputs; i++)
    {
      /*
       gegl_log_debug("processor_process", 
                 "getting image iterator for input %d", 
                 i);
      */


       GeglData *input_data = (GeglData*)g_list_nth_data(image_input_data_list,i); 
       GeglImageData *image_input_data = GEGL_IMAGE_DATA(input_data);
       GValue *input_data_value = gegl_data_get_value(input_data);
       image = (GeglImage*)g_value_get_object(input_data_value);
       gegl_image_data_get_rect(image_input_data, &rect);

       /* Get the image, if it is not NULL */ 
       if(image)
         {
           gegl_log_debug("processor_process", 
                     "input value image is %p", 
                     image);


           iters[i + num_outputs] = g_object_new (GEGL_TYPE_IMAGE_ITERATOR, 
                                                  "image", image,
                                                  "area", &rect,
                                                  NULL);  

           gegl_image_iterator_first (iters[i + num_outputs]);
         }
       else
         iters[i + num_outputs] = NULL;
    }

  /* Get the height and width of dest rect we want to process */

  image = (GeglImage*)g_value_get_object(value);
  gegl_image_data_get_rect(image_data, &rect);
  width = rect.w;
  height = rect.h;

  gegl_log_debug("processor_process", "width height %d %d", width, height);

  /* Now iterate over the scanlines */
  for(j=0; j < height; j++)
    {
      /*
      gegl_log_debug("processor_process", 
                "doing scanline %d", 
                j);
                */

      /* Call the subclass scanline func. */
      (self->func)(self->op, iters, width);

      /* Advance all the scanlines. */
      for (i = 0; i < num_inputs + num_outputs; i++)
        {
          if(iters[i])
            gegl_image_iterator_next(iters[i]);
        }

    } 

  /* Free the iterators */
  for (i = 0; i < num_inputs + num_outputs; i++)
    {
      if (iters[i])
        g_object_unref (iters[i]); 
    }

  /* Free the array of iterator pointers */
  g_free (iters);
  g_list_free(image_output_data_list); 
  g_list_free(image_input_data_list); 
}
