#include "gegl-print.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include <stdio.h>

#define MAX_PRINTED_CHARS_PER_CHANNEL 20

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglPrintClass * klass);
static void init (GeglPrint * self, GeglPrintClass * klass);
static void finalize (GObject * gobject);

static void prepare (GeglFilter * filter, GList * attributes, GList *input_attributes);
static void finish (GeglFilter * filter, GList * attributes, GList *input_attributes);

static void print (GeglPrint * self, gchar * format, ...);

static void scanline_rgb_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_float (GeglFilter * filter, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglFilter * filter, GeglTileIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_print_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPrintClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglPrint),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_STAT_OP, 
                                     "GeglPrint", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPrintClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize= finalize;

  filter_class->prepare = prepare;
  filter_class->finish = finish;

  return;
}

static void 
init (GeglPrint * self, 
      GeglPrintClass * klass)
{
  self->buffer = NULL;
  self->use_log = TRUE;

  g_object_set(self, "num_outputs", 1, NULL);
  g_object_set(self, "num_inputs", 1, NULL);
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
prepare (GeglFilter * filter, 
         GList * attributes,
         GList * input_attributes)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglStatOp *stat_op = GEGL_STAT_OP(filter);

  GeglAttributes *src_attributes = 
    (GeglAttributes*)g_list_nth_data(input_attributes, 0); 
  GeglTile *src = (GeglTile*)g_value_get_object(src_attributes->value);

  GeglRect dest_rect;
  GeglTile *dest;
  GeglColorModel * dest_cm;
  GeglAttributes *dest_attributes = 
    (GeglAttributes*)g_list_nth_data(attributes, 0); 
  
  g_value_set_tile(dest_attributes->value, src);
  gegl_rect_copy(&dest_rect, &dest_attributes->rect);

  dest = (GeglTile*)g_value_get_object(dest_attributes->value);
  dest_cm = gegl_tile_get_color_model (dest);

  g_return_if_fail (dest_cm);

  {
    GeglColorSpace colorspace = gegl_color_model_color_space(dest_cm);
    GeglChannelDataType data_type = gegl_color_model_data_type (dest_cm);

    switch (colorspace)
      {
        case GEGL_COLOR_SPACE_RGB:
          switch (data_type)
            {
            case GEGL_U8:
              stat_op->scanline_processor->func = scanline_rgb_u8;
              break;
            case GEGL_FLOAT:
              stat_op->scanline_processor->func = scanline_rgb_float;
              break;
            case GEGL_U16:
              stat_op->scanline_processor->func = scanline_rgb_u16;
              break;
            default:
              g_warning("prepare: Can't find data_type\n");    
              break;

            }
          break;
        case GEGL_COLOR_SPACE_GRAY:
          switch (data_type)
            {
            case GEGL_U8:
              stat_op->scanline_processor->func = scanline_gray_u8;
              break;
            case GEGL_FLOAT:
              stat_op->scanline_processor->func = scanline_gray_float;
              break;
            case GEGL_U16:
              stat_op->scanline_processor->func = scanline_gray_u16;
              break;
            default:
              g_warning("prepare: Can't find data_type\n");    
              break;
            }
          break;
        default:
          g_warning("prepare: Can't find colorspace\n");    
          break;
      }
  }

  {
    gint num_channels = gegl_color_model_num_channels(dest_cm);

    gint x = dest_rect.x;
    gint y = dest_rect.y;
    gint width = dest_rect.w;
    gint height = dest_rect.h;
    
    /* Allocate a scanline char buffer for output */
    self->buffer_size = width * (num_channels + 1) * MAX_PRINTED_CHARS_PER_CHANNEL;
    self->buffer = g_new(gchar, self->buffer_size); 

    /*
    LOG_INFO("prepare", "buffer_size is %d", self->buffer_size); 
    */

    if(self->use_log)
      LOG_INFO("prepare", 
               "Printing GeglTile: %p area (x,y,w,h) = (%d,%d,%d,%d)",
               dest,x,y,width,height);
    else
      printf("Printing GeglTile: %p area (x,y,w,h) = (%d,%d,%d,%d)", 
               dest,x,y,width,height);
  }
  
}

static void 
finish (GeglFilter * filter, 
        GList * attributes,
        GList * input_attributes)
{
  GeglPrint *self = GEGL_PRINT(filter);

  /* Delete the scanline char buffer used for output */ 
  g_free(self->buffer); 
}

/**
 * gegl_print_print:
 * @self: a #GeglPrint.
 * @format: the format for output.
 *
 * Adds the formatted output to the scanline output buffer.
 *
 **/
static void 
print (GeglPrint * self, 
       gchar * format, 
       ...)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_PRINT (self));

  {
    gint written = 0;
     
    va_list args;
    va_start(args,format);

    written = g_vsnprintf(self->current, self->left, format, args); 

    va_end(args);

    self->current += written; 
    self->left -= written;
  }

/*
  LOG_DEBUG("print", "total written : %d", self->buffer_size - self->left);
*/
}

/**
 * scanline_rgb_u8:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u8 (GeglFilter * filter, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint8 *dest_data[4];
  gboolean dest_has_alpha;
  guint8 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *dest_r) ;
      print (self, "%d ", *dest_g) ;
      print (self, "%d ", *dest_b) ;
      if (dest_has_alpha)
        print (self, "%d ", *dest_alpha) ;
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      print (self, ")");

    }

  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_rgb_u8", "%s",self->buffer);
  else
    printf("%s", self->buffer);
}

/**
 * scanline_rgb_float:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_float (GeglFilter * filter, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  gfloat *dest_data[4];
  gboolean dest_has_alpha;
  gfloat *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0],
                                   (gpointer*)dest_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 
/*
  LOG_INFO("scanline_rgb_float", "%d",width);
*/

  while (width--)
    {
      /*
      LOG_INFO("scanline_rgb_float", "pixel num: %d",width);
      */
      print(self, "(");
      print(self, "%.3f ", *dest_r); 
      print(self, "%.3f ", *dest_g); 
      print(self, "%.3f ", *dest_b); 

      if (dest_has_alpha)
        {
          print(self, "%.3f ", *dest_alpha); 
        }

      dest_r++;
      dest_g++;
      dest_b++;

      if (dest_has_alpha)
        dest_alpha++;

      print(self, ")"); 
    }

  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_rgb_float","%s",self->buffer);
  else
    printf("%s", self->buffer);
}

/**
 * scanline_rgb_u16:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u16 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint16 *dest_data[4];
  gboolean dest_has_alpha;
  guint16 *dest_r, *dest_g, *dest_b, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_r = dest_data[0];
  dest_g = dest_data[1];
  dest_b = dest_data[2];
  if (dest_has_alpha)
    dest_alpha = dest_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *dest_r) ;
      print (self, "%d ", *dest_g) ;
      print (self, "%d ", *dest_b) ;
      if (dest_has_alpha)
        print (self, "%d ", *dest_alpha) ;
      dest_r++;
      dest_g++;
      dest_b++;
      if (dest_has_alpha)
        dest_alpha++;

      print (self, ")");

    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_rgb_u16","%s",self->buffer);
  else
    printf("%s", self->buffer);

}

static void 
scanline_gray_u8 (GeglFilter * filter, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint8 *dest_data[2];
  gboolean dest_has_alpha;
  guint8 *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *dest_gray) ;
      if (dest_has_alpha)
        print (self, "%d ", *dest_alpha) ;
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      print (self, ")");
    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_gray_u8", "%s",self->buffer);
  else
    printf("%s", self->buffer);
}

/**
 * scanline_gray_float:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_float (GeglFilter * filter, 
                     GeglTileIterator ** iters, 
                     gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  gfloat *dest_data[2];
  gboolean dest_has_alpha;
  gfloat *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%.3f ", *dest_gray) ;
      if (dest_has_alpha)
        print (self, "%.3f ", *dest_alpha) ;
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      print (self, ")");

    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_gray_float","%s",self->buffer);
  else
    printf("%s", self->buffer);
}

/**
 * scanline_gray_u16:
 * @filter: a #GeglFilter.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u16 (GeglFilter * filter, 
                   GeglTileIterator ** iters, 
                   gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  GeglColorModel *dest_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint16 *dest_data[2];
  gboolean dest_has_alpha;
  guint16 *dest_gray, *dest_alpha=NULL;

  dest_has_alpha = gegl_color_model_has_alpha(dest_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)dest_data);

  dest_gray = dest_data[0];
  if (dest_has_alpha)
    dest_alpha = dest_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *dest_gray) ;
      if (dest_has_alpha)
        print (self, "%d ", *dest_alpha) ;
      dest_gray++;
      if (dest_has_alpha)
        dest_alpha++;

      print (self, ")");

    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_gray_u16","%s",self->buffer);
  else
    printf("%s", self->buffer);
}
