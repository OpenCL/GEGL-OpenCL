#include "gegl-scanline-processor.h"
#include "gegl-image-data.h"
#include "gegl-image-data-iterator.h"
#include "gegl-attributes.h"
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
 * @attributes: A list of output attributes. 
 * @input_attributes: A list of input attributes. 
 *
 * Process scanlines.
 *
 **/
void 
gegl_scanline_processor_process (GeglScanlineProcessor * self, 
                                 GeglAttributes * attributes,
                                 GList * input_attributes)
{
  gint i,j;
  GeglRect rect;
  GeglImageData *image_data;
  gint width, height;
  gint num_inputs = g_list_length(input_attributes);
  gint num_outputs = attributes ? 1: 0;
  GeglAttributes *input_attrs;

  GeglImageDataIterator **iters = g_new (GeglImageDataIterator*, 
                                    num_inputs + num_outputs);

#if 1 
  LOG_DEBUG("processor_process", "%s %p", 
             G_OBJECT_TYPE_NAME(self->op), self->op); 

  LOG_DEBUG("processor_process", "inputs %d outputs %d", 
             num_inputs, num_outputs); 
#endif

  /* Get image_data iterators for outputs. */
  if (num_outputs == 1)
    {
      /*
       LOG_DEBUG("processor_process", 
                 "getting image_data iterator for output %d", 
                 i);
      */

       image_data = (GeglImageData*)g_value_get_object(attributes->value);
       gegl_rect_copy(&rect, &attributes->rect);

       /* Get the image_data, if it is not NULL */ 
       if(image_data)
         {
           LOG_DEBUG("processor_process", 
                     "output value image_data is %p", 
                     image_data);

           iters[0] = g_object_new (GEGL_TYPE_IMAGE_DATA_ITERATOR, 
                                    "image_data", image_data,
                                    "area", &rect,
                                    NULL);  

           gegl_image_data_iterator_first (iters[0]);
         }
    }

  /* Get image_data iterators for inputs. */
  for (i = 0; i < num_inputs; i++)
    {
      /*
       LOG_DEBUG("processor_process", 
                 "getting image_data iterator for input %d", 
                 i);
      */

       input_attrs = (GeglAttributes*)g_list_nth_data(input_attributes,i); 
       image_data = (GeglImageData*)g_value_get_object(input_attrs->value);
       gegl_rect_copy(&rect, &input_attrs->rect);

       /* Get the image_data, if it is not NULL */ 
       if(image_data)
         {
           LOG_DEBUG("processor_process", 
                     "input value image_data is %p", 
                     image_data);


           iters[i + num_outputs] = g_object_new (GEGL_TYPE_IMAGE_DATA_ITERATOR, 
                                                  "image_data", image_data,
                                                  "area", &rect,
                                                  NULL);  

           gegl_image_data_iterator_first (iters[i + num_outputs]);
         }
       else
         iters[i + num_outputs] = NULL;
    }

  /* Get the height and width of dest rect we want to process */

  image_data = (GeglImageData*)g_value_get_object(attributes->value);
  gegl_rect_copy(&rect, &attributes->rect);
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
            gegl_image_data_iterator_next(iters[i]);
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
