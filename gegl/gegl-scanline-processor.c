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
 * @data_outputs: A list of #GeglData outputs. 
 * @data_inputs: A list of #GeglData inputs. 
 *
 * Process scanlines.
 *
 **/
void 
gegl_scanline_processor_process (GeglScanlineProcessor * self, 
                                 GList  *data_outputs,
                                 GList  *data_inputs)
{
  gint i,j;
  GeglRect rect;
  GeglImage *image;
  gint width, height;
  gint num_inputs = g_list_length(data_inputs);
  gint num_outputs = g_list_length(data_outputs);
  GeglData *data = g_list_nth_data(data_outputs,0);
  GValue *value = gegl_data_get_value(data);
  GeglImageData *image_data = GEGL_IMAGE_DATA(data);
  GeglImageIterator **iters = g_new (GeglImageIterator*, num_inputs + num_outputs);

#if 1 
  LOG_DEBUG("processor_process", "%s %p", 
             G_OBJECT_TYPE_NAME(self->op), self->op); 

  LOG_DEBUG("processor_process", "inputs %d outputs %d", 
             num_inputs, num_outputs); 
#endif

  /* Get image iterators for outputs. */
  if (num_outputs == 1)
    {
      /*
       LOG_DEBUG("processor_process", 
                 "getting image iterator for output %d", 
                 i);
      */

       image = (GeglImage*)g_value_get_object(value);
       gegl_image_data_get_rect(image_data, &rect);

       /* Get the image, if it is not NULL */ 
       if(image)
         {
           LOG_DEBUG("processor_process", 
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
       LOG_DEBUG("processor_process", 
                 "getting image iterator for input %d", 
                 i);
      */


       GeglData *data_input = (GeglData*)g_list_nth_data(data_inputs,i); 
       GeglImageData *image_data_input = GEGL_IMAGE_DATA(data_input);
       GValue *data_input_value = gegl_data_get_value(data_input);
       image = (GeglImage*)g_value_get_object(data_input_value);
       gegl_image_data_get_rect(image_data_input, &rect);

       /* Get the image, if it is not NULL */ 
       if(image)
         {
           LOG_DEBUG("processor_process", 
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
}
