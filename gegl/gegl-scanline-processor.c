#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-utils.h"

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
                                 GList * requests)
{
  GeglOpRequest * request;
  gint i,j;
  gint width, height;
  gint num_requests = g_list_length(requests);
  GeglTileIterator **iters = g_new (GeglTileIterator*, num_requests);

  LOG_DEBUG("processor_process", 
            "op_impl %x is %s", 
            (guint)self->op_impl, G_OBJECT_TYPE_NAME(self->op_impl));

  /* Get tile iterators for dest and sources. */
  for (i = 0; i < num_requests; i++)
    {
       LOG_DEBUG("processor_process", 
                 "getting tile iterator for input %d", 
                 i);

       request = (GeglOpRequest*)g_list_nth_data(requests,i); 

       /* Get the tile, if it is not NULL */ 
       if(request->tile)
         {
           LOG_DEBUG("processor_process", 
                     "tile is %x", 
                     (guint)request->tile);

           iters[i] = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                    "tile", request->tile,
                                    "area", &request->rect,
                                    NULL);  

           gegl_tile_iterator_first (iters[i]);
         }
       else
         iters[i] = NULL;
    }

  /* Get the height and width of dest rect we want to process */
  request = (GeglOpRequest*)g_list_nth_data(requests,0); 
  width = request->rect.w;
  height = request->rect.h;

  LOG_DEBUG("processor_process", "width height %d %d", width, height);

  /* Now iterate over the scanlines */
  for(j=0; j < height; j++)
    {
      LOG_DEBUG("processor_process", 
                "doing scanline %d", 
                j);

      /* Call the subclass scanline func. */
      (self->func)(self->op_impl, iters, width);

      /* Advance all the scanlines. */
      for (i = 0; i < num_requests; i++)
        {
          if(iters[i])
            gegl_tile_iterator_next(iters[i]);
        }

    } 

  /* Free the iterators */
  for (i = 0; i < num_requests; i++)
    {
      if (iters[i])
        g_object_unref (G_OBJECT (iters[i])); 
    }

  /* Free the array of iterator pointers */
  g_free (iters);
}
