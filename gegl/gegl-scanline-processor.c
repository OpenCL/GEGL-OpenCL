#include "gegl-scanline-processor.h"
#include "gegl-image-buffer.h"
#include "gegl-image-buffer-data.h"
#include "gegl-image-buffer-iterator.h"
#include "gegl-data.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"

static void class_init (GeglScanlineProcessorClass * klass);
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
        (GInstanceInitFunc) NULL,
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

/**
 * gegl_scanline_processor_process:
 * @self: a #GeglScanlineProcessor.
 * @output_data_list: A list of output data_list. 
 * @input_data_list: A list of input data_list. 
 *
 * Process scanlines.
 *
 **/
void 
gegl_scanline_processor_process (GeglScanlineProcessor * self, 
                                 GList  *output_data_list,
                                 GList  *input_data_list)
{
  gint i,j;
  GeglRect rect;
  GeglImageBuffer *image_buffer;
  gint width, height;
  gint num_inputs = g_list_length(input_data_list);
  gint num_outputs = g_list_length(output_data_list);
  GeglData *data = g_list_nth_data(output_data_list,0);
  GeglImageBufferData *image_buffer_data = GEGL_IMAGE_BUFFER_DATA(data);
  GeglImageBufferIterator **iters = g_new (GeglImageBufferIterator*, 
                                    num_inputs + num_outputs);

#if 1 
  LOG_DEBUG("processor_process", "%s %p", 
             G_OBJECT_TYPE_NAME(self->op), self->op); 

  LOG_DEBUG("processor_process", "inputs %d outputs %d", 
             num_inputs, num_outputs); 
#endif

  /* Get image_buffer iterators for outputs. */
  if (num_outputs == 1)
    {
      /*
       LOG_DEBUG("processor_process", 
                 "getting image_buffer iterator for output %d", 
                 i);
      */

       image_buffer = (GeglImageBuffer*)g_value_get_object(data->value);
       gegl_rect_copy(&rect, &image_buffer_data->rect);

       /* Get the image_buffer, if it is not NULL */ 
       if(image_buffer)
         {
           LOG_DEBUG("processor_process", 
                     "output value image_buffer is %p", 
                     image_buffer);

           iters[0] = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                                    "image_buffer", image_buffer,
                                    "area", &rect,
                                    NULL);  

           gegl_image_buffer_iterator_first (iters[0]);
         }
    }

  /* Get image_buffer iterators for inputs. */
  for (i = 0; i < num_inputs; i++)
    {
      /*
       LOG_DEBUG("processor_process", 
                 "getting image_buffer iterator for input %d", 
                 i);
      */


       GeglData *input_data = (GeglData*)g_list_nth_data(input_data_list,i); 
       GeglImageBufferData *input_image_buffer_data = GEGL_IMAGE_BUFFER_DATA(input_data);

       image_buffer = (GeglImageBuffer*)g_value_get_object(input_data->value);
       gegl_rect_copy(&rect, &input_image_buffer_data->rect);

       /* Get the image_buffer, if it is not NULL */ 
       if(image_buffer)
         {
           LOG_DEBUG("processor_process", 
                     "input value image_buffer is %p", 
                     image_buffer);


           iters[i + num_outputs] = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                                                  "image_buffer", image_buffer,
                                                  "area", &rect,
                                                  NULL);  

           gegl_image_buffer_iterator_first (iters[i + num_outputs]);
         }
       else
         iters[i + num_outputs] = NULL;
    }

  /* Get the height and width of dest rect we want to process */

  image_buffer = (GeglImageBuffer*)g_value_get_object(data->value);
  gegl_rect_copy(&rect, &image_buffer_data->rect);
  width = rect.w;
  height = rect.h;

  LOG_DEBUG("processor_process", "width height %d %d", width, height);

  /* Now iterate over the scanlines */
  for(j=0; j < height; j++)
    {
      /*
      LOG_DEBUG("processor_process", 
                "doing scanline %d", 
                j);
                */

      /* Call the subclass scanline func. */
      (self->func)(self->op, iters, width);

      /* Advance all the scanlines. */
      for (i = 0; i < num_inputs + num_outputs; i++)
        {
          if(iters[i])
            gegl_image_buffer_iterator_next(iters[i]);
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
}
