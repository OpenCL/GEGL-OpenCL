#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
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

void 
gegl_scanline_processor_process (GeglScanlineProcessor * self, 
                                 GList * output_values,
                                 GList * input_values)
{
  GValue * value;
  gint i,j;
  GeglRect rect;
  GeglTile *tile;
  gint width, height;
  gint num_inputs = g_list_length(input_values);
  gint num_outputs = g_list_length(output_values);
  GeglTileIterator **iters = g_new (GeglTileIterator*, 
                                    num_inputs + num_outputs);

  /*
  LOG_DEBUG("processor_process", "%s %x", 
             G_OBJECT_TYPE_NAME(self->op), (guint)self->op); 

  LOG_DEBUG("processor_process", "inputs %d outputs %d", 
             num_inputs, num_outputs); 
  */           

  /* Get tile iterators for dests. */
  for (i = 0; i < num_outputs; i++, j++)
    {
  /*
       LOG_DEBUG("processor_process", 
                 "getting tile iterator for output %d", 
                 i);
  */           


       value = (GValue*)g_list_nth_data(output_values,i); 
       tile = g_value_get_image_data_tile(value);
       g_value_get_image_data_rect(value, &rect);

       /* Get the tile, if it is not NULL */ 
       if(tile)
         {
  /*
           LOG_DEBUG("processor_process", 
                     "tile is %x", 
                     (guint)tile);
  */           

           iters[i] = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                    "tile", tile,
                                    "area", &rect,
                                    NULL);  

           gegl_tile_iterator_first (iters[i]);
         }
       else
         iters[i] = NULL;
    }

  /* Get tile iterators for dests. */
  for (i = 0; i < num_inputs; i++)
    {
  /*
       LOG_DEBUG("processor_process", 
                 "getting tile iterator for input %d", 
                 i);
  */           

       value = (GValue*)g_list_nth_data(input_values,i); 
       tile = g_value_get_image_data_tile(value);
       g_value_get_image_data_rect(value, &rect);

       /* Get the tile, if it is not NULL */ 
       if(tile)
         {
  /*
           LOG_DEBUG("processor_process", 
                     "tile is %x", 
                     (guint)tile);
  */           


           iters[i + num_outputs] = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                    "tile", tile,
                                    "area", &rect,
                                    NULL);  

           gegl_tile_iterator_first (iters[i + num_outputs]);
         }
       else
         iters[i + num_outputs] = NULL;
    }

  /* Get the height and width of dest rect we want to process */
  value = (GValue*)g_list_nth_data(output_values,0); 
  tile = g_value_get_image_data_tile(value);
  g_value_get_image_data_rect(value, &rect);

  width = rect.w;
  height = rect.h;

  /*
  LOG_DEBUG("processor_process", "width height %d %d", width, height);
  */           

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
            gegl_tile_iterator_next(iters[i]);
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
