#include "gegl-print.h"
#include "gegl-scanline-processor.h"
#include "gegl-image.h"
#include "gegl-image-data.h"
#include "gegl-image-iterator.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"
#include "gegl-value-types.h"
#include "gegl-data.h"
#include "gegl-param-specs.h"
#include <stdio.h>

#define MAX_PRINTED_CHARS_PER_CHANNEL 20

static void class_init (GeglPrintClass * klass);
static void init (GeglPrint * self, GeglPrintClass * klass);
static void prepare (GeglFilter * filter);
static void finish (GeglFilter * filter);

static GeglScanlineFunc get_scanline_func(GeglPipe * pipe, GeglColorSpaceType space, GeglChannelSpaceType type);
static void print (GeglPrint * self, gchar * format, ...);

static void print_float (GeglFilter * filter, GeglImageIterator ** iters, gint width);

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

      type = g_type_register_static (GEGL_TYPE_PIPE, 
                                     "GeglPrint", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglPrintClass * klass)
{
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglPipeClass *pipe_class = GEGL_PIPE_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  pipe_class->get_scanline_func = get_scanline_func;

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
}

static void 
prepare (GeglFilter * filter) 
{
  GeglPrint *self = GEGL_PRINT(filter);
  GList * output_data_list = gegl_op_get_output_data_list(GEGL_OP(self));
  GeglData *src_data; 
  GValue *src_value;
  GeglImageData* src_image_data;
  GeglColorModel *src_cm;
  GeglImage *src;
  GeglRect rect;
  gint num_channels;

  GEGL_FILTER_CLASS(parent_class)->prepare(filter);

  src_data = (GeglData*)g_list_nth_data(output_data_list, 0); 
  src_image_data = GEGL_IMAGE_DATA(src_data);
  src_value = gegl_data_get_value(src_data);

  src = (GeglImage*)g_value_get_object(src_value);
  src_cm = gegl_image_get_color_model (src);
  num_channels = gegl_color_model_num_channels(src_cm);

  gegl_rect_copy(&rect, &src_image_data->rect);

  {
    gint x = rect.x;
    gint y = rect.y;
    gint width = rect.w;
    gint height = rect.h;
    
    /* Allocate a scanline char buffer for output */
    self->buffer_size = width * (num_channels + 1) * MAX_PRINTED_CHARS_PER_CHANNEL;
    self->buffer = g_new(gchar, self->buffer_size); 

    /*
    LOG_INFO("prepare", "buffer_size is %d", self->buffer_size); 
    */

    if(self->use_log)
      LOG_INFO("prepare", 
               "Printing GeglImage: %p area (x,y,w,h) = (%d,%d,%d,%d)",
               src,x,y,width,height);
    else
      printf("Printing GeglImage: %p area (x,y,w,h) = (%d,%d,%d,%d)", 
               src,x,y,width,height);
  }
}

static void 
finish (GeglFilter * filter) 
{
  GeglPrint *self = GEGL_PRINT(filter);
  g_free(self->buffer); 
}

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

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  NULL, 
  print_float, 
  NULL 
};

static GeglScanlineFunc
get_scanline_func(GeglPipe *pipe,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void 
print_float (GeglFilter * filter, 
             GeglImageIterator ** iters, 
             gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);
  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(iters[0]);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(iters[0]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gint alpha_mask = 0x0;

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  self->current = self->buffer;
  self->left = self->buffer_size; 

  {
    gfloat *a0 = (a_color_chans > 0) ? a[0]: NULL;   
    gfloat *a1 = (a_color_chans > 1) ? a[1]: NULL;
    gfloat *a2 = (a_color_chans > 2) ? a[2]: NULL;

    switch(a_color_chans)
      {
        case 3: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, "%.3f ", *a1++);
                print(self, "%.3f ", *a2++); 
                print(self, "%.3f ", *aa++); 
                print(self, ")"); 
              }
          else
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, "%.3f ", *a1++);
                print(self, "%.3f ", *a2++); 
                print(self, ")"); 
              }
          break;
        case 2: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, "%.3f ", *a1++);
                print(self, "%.3f ", *aa++); 
                print(self, ")"); 
              }
          else
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, "%.3f ", *a1++);
                print(self, ")"); 
              }
          break;
        case 1: 
          if(alpha_mask == GEGL_A_ALPHA)
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, "%.3f ", *aa++); 
                print(self, ")"); 
              }
          else
            while(width--)                                                        
              {                                                                   
                print(self, "(");
                print(self, "%.3f ", *a0++);
                print(self, ")"); 
              }
          break;
      }

    print(self,"%c", (char)0);

    if(self->use_log)
      LOG_INFO("print_float","%s",self->buffer);
    else
      printf("%s", self->buffer);
  }

  g_free(a);
}
