#include "gegl-print.h"
#include "gegl-scanline-processor.h"
#include "gegl-tile.h"
#include "gegl-tile-iterator.h"
#include "gegl-color.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include <stdio.h>

#define MAX_PRINTED_CHARS_PER_CHANNEL 20

static void class_init (GeglPrintClass * klass);
static void init (GeglPrint * self, GeglPrintClass * klass);
static void finalize (GObject * gobject);

static void prepare (GeglOp * self_op, GList * requests);
static void finish (GeglOp * self_op, GList * requests);

static void print (GeglPrint * self, gchar * format, ...);

static void scanline_rgb_u8 (GeglOp * self_op, GeglTileIterator ** iters, gint width);
static void scanline_rgb_float (GeglOp * self_op, GeglTileIterator ** iters, gint width);
static void scanline_rgb_u16 (GeglOp * self_op, GeglTileIterator ** iters, gint width);
static void scanline_gray_u8 (GeglOp * self_op, GeglTileIterator ** iters, gint width);
static void scanline_gray_float (GeglOp * self_op, GeglTileIterator ** iters, gint width);
static void scanline_gray_u16 (GeglOp * self_op, GeglTileIterator ** iters, gint width);

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
  GeglOpClass *op_class = GEGL_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize= finalize;

  op_class->prepare = prepare;
  op_class->finish = finish;

  return;
}

static void 
init (GeglPrint * self, 
      GeglPrintClass * klass)
{
  self->buffer = NULL;
  self->use_log = TRUE;

  {
    GeglNode * self_node = GEGL_NODE(self); 
    GList * inputs = g_list_append(NULL, NULL);
    self_node->num_inputs = 1;
    gegl_node_set_inputs(self_node, inputs);
    g_list_free(inputs);
  }
  return;
}

static void
finalize(GObject *gobject)
{
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void 
prepare (GeglOp * self_op, 
         GList * requests)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglStatOp *stat_op = GEGL_STAT_OP(self_op);

  GeglOpRequest *src_request = (GeglOpRequest*)g_list_nth_data(requests,0); 
  GeglColorModel * src_cm = gegl_tile_get_color_model (src_request->tile);
  g_return_if_fail (src_cm);

  {
    GeglColorSpace colorspace = gegl_color_model_color_space(src_cm);
    GeglChannelDataType data_type = gegl_color_model_data_type (src_cm);

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
    gint num_channels = gegl_color_model_num_channels(src_cm);
    gint x = src_request->rect.x;
    gint y = src_request->rect.y;
    gint width = src_request->rect.w;
    gint height = src_request->rect.w;
    
    /* Allocate a scanline char buffer for output */
    self->buffer_size = width * (num_channels + 1) * MAX_PRINTED_CHARS_PER_CHANNEL;
    self->buffer = g_new(gchar, self->buffer_size); 

    if(self->use_log)
      LOG_INFO("prepare", 
               "Printing GeglTile: %x area (x,y,w,h) = (%d,%d,%d,%d)",
               (guint)(src_request->tile),x,y,width,height);
    else
      printf("Printing GeglTile: %x area (x,y,w,h) = (%d,%d,%d,%d)", 
               (guint)(src_request->tile),x,y,width,height);
  }
}

static void 
finish (GeglOp * self_op, 
        GList * requests)
{
  GeglPrint *self = GEGL_PRINT(self_op);

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
}

/**
 * scanline_rgb_u8:
 * @self_op: a #GeglOp.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u8 (GeglOp * self_op, 
                 GeglTileIterator ** iters, 
                 gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[1]);

  guint8 *src_data[4];
  gboolean src_has_alpha;
  guint8 *src_r, *src_g, *src_b, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[1], (gpointer*)src_data);

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *src_r) ;
      print (self, "%d ", *src_g) ;
      print (self, "%d ", *src_b) ;
      if (src_has_alpha)
        print (self, "%d ", *src_alpha) ;
      src_r++;
      src_g++;
      src_b++;
      if (src_has_alpha)
        src_alpha++;

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
 * @self_op: a #GeglOp.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_float (GeglOp * self_op, 
                    GeglTileIterator ** iters, 
                    gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[0]);

  gfloat *src_data[4];
  gboolean src_has_alpha;
  gfloat *src_r, *src_g, *src_b, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0],
                                   (gpointer*)src_data);

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 

  while (width--)
    {
      print(self, "(");
      print(self, "%.3f ", *src_r); 
      print(self, "%.3f ", *src_g); 
      print(self, "%.3f ", *src_b); 

      if (src_has_alpha)
        {
          print(self, "%.3f ", *src_alpha); 
        }

      src_r++;
      src_g++;
      src_b++;

      if (src_has_alpha)
        src_alpha++;

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
 * @self_op: a #GeglOp.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_rgb_u16 (GeglOp * self_op, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint16 *src_data[4];
  gboolean src_has_alpha;
  guint16 *src_r, *src_g, *src_b, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)src_data);

  src_r = src_data[0];
  src_g = src_data[1];
  src_b = src_data[2];
  if (src_has_alpha)
    src_alpha = src_data[3];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *src_r) ;
      print (self, "%d ", *src_g) ;
      print (self, "%d ", *src_b) ;
      if (src_has_alpha)
        print (self, "%d ", *src_alpha) ;
      src_r++;
      src_g++;
      src_b++;
      if (src_has_alpha)
        src_alpha++;

      print (self, ")");

    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_rgb_u16","%s",self->buffer);
  else
    printf("%s", self->buffer);

}

static void 
scanline_gray_u8 (GeglOp * self_op, 
                  GeglTileIterator ** iters, 
                  gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint8 *src_data[2];
  gboolean src_has_alpha;
  guint8 *src_gray, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)src_data);

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *src_gray) ;
      if (src_has_alpha)
        print (self, "%d ", *src_alpha) ;
      src_gray++;
      if (src_has_alpha)
        src_alpha++;

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
 * @self_op: a #GeglOp.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_float (GeglOp * self_op, 
                     GeglTileIterator ** iters, 
                     gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[0]);

  gfloat *src_data[2];
  gboolean src_has_alpha;
  gfloat *src_gray, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)src_data);

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%.3f ", *src_gray) ;
      if (src_has_alpha)
        print (self, "%.3f ", *src_alpha) ;
      src_gray++;
      if (src_has_alpha)
        src_alpha++;

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
 * @self_op: a #GeglOp.
 * @iters: #GeglTileIterators array. 
 * @width: width of scanline. 
 *
 * Processes a scanline.
 *
 **/
static void 
scanline_gray_u16 (GeglOp * self_op, 
                   GeglTileIterator ** iters, 
                   gint width)
{
  GeglPrint *self = GEGL_PRINT(self_op);
  GeglColorModel *src_cm = gegl_tile_iterator_get_color_model(iters[0]);

  guint16 *src_data[2];
  gboolean src_has_alpha;
  guint16 *src_gray, *src_alpha=NULL;

  src_has_alpha = gegl_color_model_has_alpha(src_cm); 

  gegl_tile_iterator_get_current (iters[0], (gpointer*)src_data);

  src_gray = src_data[0];
  if (src_has_alpha)
    src_alpha = src_data[1];

  self->current = self->buffer;
  self->left = self->buffer_size; 
  
  
  while (width--)
    {
      print (self, "(");
      print (self, "%d ", *src_gray) ;
      if (src_has_alpha)
        print (self, "%d ", *src_alpha) ;
      src_gray++;
      if (src_has_alpha)
        src_alpha++;

      print (self, ")");

    }
  print(self,"%c", (char)0);

  if(self->use_log)
    LOG_INFO("scanline_gray_u16","%s",self->buffer);
  else
    printf("%s", self->buffer);
}
