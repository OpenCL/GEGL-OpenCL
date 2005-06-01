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
#include <string.h>

#define MAX_PRINTED_CHARS_PER_CHANNEL 20

static void class_init (GeglPrintClass * klass);
static void init (GeglPrint * self, GeglPrintClass * klass);
static void prepare (GeglFilter * filter);
static void finish (GeglFilter * filter);

static GeglScanlineFunc get_scanline_function(GeglPipe * pipe, GeglColorModel *cm);
static void print (GeglPrint * self, gchar * format, ...);

static void print_float (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);

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
        NULL
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

  pipe_class->get_scanline_function = get_scanline_function;

  filter_class->prepare = prepare;
  filter_class->finish = finish;
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

  GEGL_FILTER_CLASS(parent_class)->prepare(filter);

  {
    GeglData *src_data = gegl_op_get_input_data(GEGL_OP(self), "source");
    GValue *src_value = gegl_data_get_value(src_data);
    GeglImage *src = (GeglImage*)g_value_get_object(src_value);
    GeglColorModel *src_cm = gegl_image_get_color_model (src);
    gint num_channels = gegl_color_model_num_channels(src_cm);

    GeglRect rect;
    gegl_image_data_get_rect(GEGL_IMAGE_DATA(src_data), &rect);

    {
      gint x = rect.x;
      gint y = rect.y;
      gint width = rect.w;
      gint height = rect.h;

      /* Allocate a scanline char buffer for output */
      self->buffer_size = width * (num_channels + 1) * MAX_PRINTED_CHARS_PER_CHANNEL;
      self->buffer = g_new(gchar, self->buffer_size);

      /*
      gegl_log_info(__FILE__, __LINE__, "prepare", "buffer_size is %d", self->buffer_size);
      */

      if(self->use_log)
        gegl_log_info(__FILE__, __LINE__,"prepare",
                 "Printing GeglImage: %p area (x,y,w,h) = (%d,%d,%d,%d)",
                 src,x,y,width,height);
      else
        printf("Printing GeglImage: %p area (x,y,w,h) = (%d,%d,%d,%d)",
                 (gpointer)src,x,y,width,height);
    }
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
  gegl_log_debug(__FILE__, __LINE__, "print",
                 "total written : %d", self->buffer_size - self->left);
*/
}

static GeglScanlineFunc
get_scanline_function(GeglPipe * pipe,
                      GeglColorModel *cm)
{
  gchar *name = gegl_color_model_name(cm);

  if(!strcmp(name, "rgb-float"))
    return print_float;
  else if(!strcmp(name, "rgba-float"))
    return print_float;

  return NULL;
}


static void
print_float (GeglFilter * filter,
             GeglScanlineProcessor *processor,
             gint width)
{
  GeglPrint *self = GEGL_PRINT(filter);

  GeglImageIterator *source =
    gegl_scanline_processor_lookup_iterator(processor, "source");
  gfloat **a = (gfloat**)gegl_image_iterator_color_channels(source);
  gfloat *aa = (gfloat*)gegl_image_iterator_alpha_channel(source);
  gint a_color_chans = gegl_image_iterator_get_num_colors(source);
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
      gegl_log_info(__FILE__, __LINE__,"print_float","%s",self->buffer);
    else
      printf("%s", self->buffer);
  }

  g_free(a);
}
