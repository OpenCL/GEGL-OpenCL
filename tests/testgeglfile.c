#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-image-buffer.h"
#include "gegl-image-iterator.h"
#include "gegl-composite-op.h"
#include "gegl-composite-premult-op.h"
#include "gegl-utils.h"
#include "gegl-fill-op.h"
#include "gegl-color-model.h"
#include "gegl-color-model-rgb.h"
 
#if 1
#include "gegl-min-op.h"
#include "gegl-max-op.h"
#include "gegl-diff-op.h"
#include "gegl-subtract-op.h"
#include "gegl-screen-op.h"
#include "gegl-dark-op.h"
#include "gegl-add-op.h"
#include "gegl-light-op.h"
#include "gegl-copychan-op.h"
#include "gegl-copy-op.h"
#endif

#include "gegl-mult-op.h"
#include "gegl-premult-op.h"
#include "gegl-unpremult-op.h"
#include "gegl-types.h"

#define HI_CLAMP(x,hi)     (x)>(hi)?(hi):(x) 

static void
create_preview(GtkWidget **window, 
               GtkWidget **preview, 
               int width, 
               int height, 
               gchar *name)
{
        
  /* lets create the container window first */
  *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(*window), name);
  gtk_container_set_border_width(GTK_CONTAINER(*window), 10);
  
  /* now we can create the preview */
  *preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(*preview), width, height); /*****/
  gtk_container_add(GTK_CONTAINER(*window), *preview);

}

static void
display_image(GtkWidget *window, 
              GtkWidget *preview, 
              GeglImageBuffer* image_buffer,
              GeglRect rect,
	      GeglChannelDataType data_type)
{
  gint            w, h, num_chans;
  gfloat          **data_ptrs1;
  guint8          **data_ptrs2;
  guint16         **data_ptrs3;
  guint16         **data_ptrs4;
  int             i, j;
  guchar          *tmp;
  num_chans = gegl_color_model_num_channels(
                  gegl_image_color_model (
		  GEGL_IMAGE(image_buffer)));

  data_ptrs1 = (gfloat**) g_malloc(sizeof(gfloat*) * num_chans);
  data_ptrs2 = (guint8**) g_malloc(sizeof(guint8*) * num_chans);
  data_ptrs3 = (guint16**) g_malloc(sizeof(guint16*) * num_chans);
  data_ptrs4 = (guint16**) g_malloc(sizeof(guint16*) * num_chans);
  h = rect.h;
  w = rect.w;
  tmp = g_new(char, 3*w);

  { 
    GeglImageIterator *iterator = 
      gegl_image_iterator_new(GEGL_IMAGE(image_buffer), &rect);

    for(i=0; i<h; i++)
      {
	switch (data_type)
	  {
	  case GEGL_FLOAT:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs1);

	    /* convert the data from float -> uint8 */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = ROUND(CLAMP(data_ptrs1[0][j],0.0,1.0) * 255);
		tmp[1 + 3*j] = ROUND(CLAMP(data_ptrs1[1][j],0.0,1.0) * 255);
		tmp[2 + 3*j] = ROUND(CLAMP(data_ptrs1[2][j],0.0,1.0) * 255);
	      }
	    break;
	  case GEGL_U8:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs2);

	    /* no need to convert */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = data_ptrs2[0][j];
		tmp[1 + 3*j] = data_ptrs2[1][j];
		tmp[2 + 3*j] = data_ptrs2[2][j];
	      }
	    break;
	  case GEGL_U16:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs3);

	    /* convert the data from uint16 -> uint8 */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = ROUND((data_ptrs3[0][j]/65535.0) * 255);
		tmp[1 + 3*j] = ROUND((data_ptrs3[1][j]/65535.0) * 255);
		tmp[2 + 3*j] = ROUND((data_ptrs3[2][j]/65535.0) * 255);
	      }
	    break;
	  case GEGL_U16_4:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs4);

	    /* convert the data from u16_4k -> uint8 */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = ROUND((HI_CLAMP(data_ptrs4[0][j],4095)/4095.0) * 255);
		tmp[1 + 3*j] = ROUND((HI_CLAMP(data_ptrs4[1][j],4095)/4095.0) * 255);
		tmp[2 + 3*j] = ROUND((HI_CLAMP(data_ptrs4[2][j],4095)/4095.0) * 255);
	      }
	    break;  
	  default:
	   break;  

	  }       
	gtk_preview_draw_row(GTK_PREVIEW(preview), tmp, 0, h-1-i, w);
	gegl_image_iterator_next_scanline(iterator);  
      }
    gegl_object_unref (GEGL_OBJECT(iterator));
  }

  g_free(data_ptrs1); 
  g_free(data_ptrs2); 
  g_free(data_ptrs3); 
  g_free(data_ptrs4); 
  g_free(tmp); 

  /* display the window */
  gtk_widget_show_all(window);

}       

void 
premultiply_buffer(GeglImageBuffer *buffer,
                   GeglRect *rect)
{
  GeglImage *image = GEGL_IMAGE(buffer);
  GeglImage *op = GEGL_IMAGE(gegl_premult_op_new(image));
  gegl_image_get_pixels(op, image, rect);
  gegl_object_unref (GEGL_OBJECT(op));
}


void 
unpremultiply_buffer(GeglImageBuffer *buffer,
                     GeglRect *rect)
{
  GeglImage *image = GEGL_IMAGE(buffer);
  GeglImage *op = GEGL_IMAGE(gegl_unpremult_op_new(image));
  gegl_image_get_pixels(op, image,rect);
  gegl_object_unref (GEGL_OBJECT(op));
}

void
test_copy_op (GeglImageBuffer ** src_image_buffer,
    		guint * src_width,
   		guint * src_height,
    		GeglRect *src_rect)
{
  GeglRect               dest_rect;
  GeglImageBuffer       *dest_image_buffer;
  GeglColorModel        *dest_color_model;
  GeglChannelDataType    data_type;
  GeglImage 		*op;
  gint                   num_chans;
  gboolean               has_alpha;
  gint                   width, height;
  GtkWidget       	*dest_window;
  GtkWidget       	*dest_preview;
  GeglImage 		*s;
  GeglImage		*d;


  /* create the destination , same size as src 1 for now */
  width = src_width[0];
  height = src_height[0];
  dest_color_model = gegl_image_color_model (
                     GEGL_IMAGE(src_image_buffer[0]));  

  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                            width, height);
  data_type = gegl_color_model_data_type (dest_color_model);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  has_alpha = gegl_color_model_has_alpha (dest_color_model);
  gegl_rect_set (&dest_rect, 0, 0, width, height);

  /* premultiplied composite */
  s = GEGL_IMAGE (src_image_buffer[0]);
  d =  GEGL_IMAGE (dest_image_buffer);

  op = GEGL_IMAGE (gegl_copy_op_new (s));

  gegl_image_get_pixels (op, d, &dest_rect);   
  gegl_object_unref (GEGL_OBJECT(op));

  /* display the destination */ 
  create_preview (&dest_window, &dest_preview, 
      width, height, "copy");

  display_image (dest_window, dest_preview, 
      dest_image_buffer,  dest_rect,
      data_type);
}


void
test_copychan_op (GeglImageBuffer ** src_image_buffer,
    guint * src_width,
    guint * src_height,
    GeglRect *src_rect)
{
  GeglRect               dest_rect;
  GeglImageBuffer       *dest_image_buffer;
  GeglColorModel        *dest_color_model;
  GeglChannelDataType    data_type;
  GeglImage 		*op;
  gint                   num_chans;
  gboolean               has_alpha;
  gint                   width, height;
  GtkWidget       	*dest_window;
  GtkWidget       	*dest_preview;
  GeglImage 		*s1;
  GeglImage		*d;
  gint			index[4] = {0, 1, 2, 3};


  /* create the destination , same size as src 1 for now */
  width = src_width[0];
  height = src_height[0];
  dest_color_model = gegl_image_color_model (
                     GEGL_IMAGE(src_image_buffer[0]));  

  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                            width, height);
  data_type = gegl_color_model_data_type (dest_color_model);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  has_alpha = gegl_color_model_has_alpha (dest_color_model);
  gegl_rect_set (&dest_rect, 0, 0, width, height);

  /* premultiplied composite */
  s1 = GEGL_IMAGE (src_image_buffer[0]);
  d =  GEGL_IMAGE (dest_image_buffer);

  op = GEGL_IMAGE (gegl_copychan_op_new (index, 4));

  gegl_image_get_pixels (op, s1, &src_rect[0]);   
  gegl_object_unref (GEGL_OBJECT(op));

  /* display the destination */ 
  create_preview (&dest_window, &dest_preview, 
      width, height, "r=r g=g  b=b");

  display_image (dest_window, dest_preview, 
      src_image_buffer[0],  src_rect[0],
      data_type);
}


void
test_composite_ops( GeglImageBuffer ** src_image_buffer,
                    guint * src_width,
                    guint * src_height,
                    GeglRect *src_rect )
{
  GeglRect               dest_rect;
  GeglImageBuffer       *dest_image_buffer;
  GeglColorModel        *dest_color_model;
  GeglChannelDataType    data_type;
  GeglImage 		*op;
  gint                   num_chans;
  gboolean               has_alpha;
  gint                   i;
  gint                   width, height;
  GtkWidget       	*dest_window[6];
  GtkWidget       	*dest_preview[6];
  gchar	          	*mode_names[] = {"REPLACE", 
                                     "OVER", 
                                     "IN", 
                                     "OUT", 
                                     "ATOP", 
                                     "XOR"};


  /* create the destination , same size as src 1 for now */
  width = src_width[0];
  height = src_height[0];
  dest_color_model = gegl_image_color_model (
                                GEGL_IMAGE(src_image_buffer[0]));  

  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                             width, height);
  data_type = gegl_color_model_data_type (dest_color_model);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  has_alpha = gegl_color_model_has_alpha (dest_color_model);
  gegl_rect_set (&dest_rect, 0, 0, width, height);

    /* premultiplied composite */
#if 1 
  for (i = 0; i< 6; i++) 
    {
      GeglImage *s1 = GEGL_IMAGE (src_image_buffer[0]);
      GeglImage *s2 = GEGL_IMAGE (src_image_buffer[1]);
      GeglImage *d =  GEGL_IMAGE (dest_image_buffer);

      op = GEGL_IMAGE (gegl_composite_premult_op_new (s1, s2,i));

      gegl_image_get_pixels (op, d, &dest_rect);   
      gegl_object_unref (GEGL_OBJECT(op));

      /* display the destination */ 
      create_preview (&dest_window[i], &dest_preview[i], 
	              width, height, mode_names[i]);

      display_image (dest_window[i], dest_preview[i], 
	             dest_image_buffer,  dest_rect,
	  	     data_type);
    }
#endif

    /* unpremultiplied composite */
#if 0 
  for (i = 0; i< 6; i++) 
    {
      GeglImage *s1 = GEGL_IMAGE (src_image_buffer[0]);
      GeglImage *s2 = GEGL_IMAGE (src_image_buffer[1]);
      GeglImage *d = GEGL_IMAGE (dest_image_buffer);

      op = GEGL_IMAGE(gegl_composite_op_new (s1,s2,i));
      gegl_image_get_pixels (op,d,&dest_rect);   
      gegl_object_unref (GEGL_OBJECT(op));

      /* unpremultiplied dest, so premultiply on display */
      if (has_alpha)
        premultiply_buffer(dest_image_buffer, &dest_rect);

      /* display the destination */ 
      create_preview (&dest_window[i], &dest_preview[i], 
	              width, height, mode_names[i]);

      display_image (dest_window[i], dest_preview[i], 
	             dest_image_buffer,  dest_rect,
	  	     data_type);
    }
#endif

  gegl_object_unref (GEGL_OBJECT (dest_image_buffer));
}

typedef enum
{
  POINT_OP_MIN,
  POINT_OP_MAX,
  POINT_OP_MULT,
  POINT_OP_SUBTRACT,
  POINT_OP_DIFF,
  POINT_OP_SCREEN,
  POINT_OP_DARK,
  POINT_OP_LIGHT,
  POINT_OP_PLUS
}PointOpType;

GeglImage *
point_op_factory (GeglImageBuffer * dest,
                  GeglImageBuffer **srcs,
                  PointOpType type)
{

  GeglImage *s1 = GEGL_IMAGE(srcs[0]);
  GeglImage *s2 = GEGL_IMAGE(srcs[1]);

  switch (type)
    {
    case POINT_OP_MIN:
       return GEGL_IMAGE(gegl_min_op_new (s1, s2));
    case POINT_OP_MAX:
       return GEGL_IMAGE(gegl_max_op_new (s1, s2));
    case POINT_OP_MULT:
       return GEGL_IMAGE(gegl_mult_op_new (s1, s2)); 
    case POINT_OP_SUBTRACT:
       return GEGL_IMAGE(gegl_subtract_op_new (s1, s2)); 
    case POINT_OP_DIFF:
       return GEGL_IMAGE(gegl_diff_op_new (s1, s2)); 
    case POINT_OP_SCREEN:
       return GEGL_IMAGE(gegl_screen_op_new (s1, s2)); 
    case POINT_OP_DARK:
       return GEGL_IMAGE(gegl_dark_op_new (s1, s2)); 
    case POINT_OP_LIGHT:
       return GEGL_IMAGE(gegl_light_op_new (s1, s2)); 
    case POINT_OP_PLUS:
       return GEGL_IMAGE(gegl_add_op_new (s1, s2)); 
    default:
       return NULL;
    }
}

void
test_point_ops( GeglImageBuffer ** src_image_buffer,
                    guint * src_width,
                    guint * src_height,
                    GeglRect *src_rect)
{
  GeglRect               dest_rect;
  GeglImageBuffer       *dest_image_buffer;
  GeglColorModel        *dest_color_model;
  GeglChannelDataType    data_type;
  GeglImage 		     *op;
  gint                   num_chans;
  gint                   i;
  gint                   width, height;
  GtkWidget       	*dest_window[9];
  GtkWidget       	*dest_preview[9];
  gchar                 *point_op_names[] = {"MIN", "MAX", "MULT", 
                                             "SUB", "DIFF", "SCREEN", 
                                             "DARK", "LIGHT", "PLUS"};        

  width = src_width[0];
  height = src_height[0];
  dest_color_model = gegl_image_color_model (GEGL_IMAGE(src_image_buffer[0]));  
  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                            width, height);
  data_type = gegl_color_model_data_type (dest_color_model);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  gegl_rect_set (&dest_rect, 0,0, width, height);

  for (i = 0; i < 9; i++) 
    {
      GeglRect roi;
      op = point_op_factory (dest_image_buffer, src_image_buffer, (PointOpType)i); 
      gegl_rect_set(&roi,0,0,src_width[i],src_height[i]); 
      gegl_image_get_pixels(op,GEGL_IMAGE(dest_image_buffer),&roi);

      /* display the destination */ 
      create_preview (&dest_window[i], &dest_preview[i], 
	              width, height, point_op_names[i]);
      display_image (dest_window[i], dest_preview[i], 
	             dest_image_buffer, dest_rect, data_type);
      gegl_object_unref (GEGL_OBJECT(op));
    }


  gegl_object_unref (GEGL_OBJECT (dest_image_buffer));
}

guchar *
read_tiff_image_data (TIFF *tif,
		      gint width,
		      gint height,
		      gint num_chans,
		      GeglChannelDataType data_type,
		      gboolean has_alpha)
{
  int             	i, j;
  gfloat           	*t1=NULL;
  guint8           	*t2=NULL;
  guint16           	*t3=NULL;
  guint16           	*t4=NULL;
  uint32          *image;
  guchar          *image_data;
  guchar          r,g,b,a; 
  gint            plane_size;
  gint            channel_bytes = 0;

  /* Create an appropriate image_data for passing to gegl_image_buffer */  
  switch (data_type) 
    {
    case GEGL_FLOAT:
      channel_bytes = sizeof(float);
      break;
    case GEGL_U8:
      channel_bytes = sizeof(guint8);
      break;
    case GEGL_U16:
      channel_bytes = sizeof(guint16);
      break;
    case GEGL_U16_4:
      channel_bytes = sizeof(guint16);
      break;
    default:
      break; 
    }

  image_data = (guchar*)g_malloc(width * height * channel_bytes * num_chans);

  /* Initialize some data pointers */
  switch (data_type) 
    {
    case GEGL_FLOAT:
      t1 = (gfloat *)image_data;
      break;
    case GEGL_U8:
      t2 = (guint8 *)image_data;
      break;
    case GEGL_U16:
      t3 = (guint16 *)image_data;
      break;
    case GEGL_U16_4:
      t4 = (guint16 *)image_data;
      break;
    default:
      break; 
    }

  /* Read the tiff data into abgr packed uint32s. */  

  image = (uint32*) _TIFFmalloc(width* height * sizeof(uint32));
  TIFFReadRGBAImage(tif, width, height, image, 0);

  /* Convert the abgr packed uint32s to image_data format 
     suitable for passing to a GeglImageBuffer. */

  j=0;
  plane_size = width * height;
  for(i=0; i<plane_size; i++)
    {
      r = TIFFGetR(image[i]);
      g = TIFFGetG(image[i]);
      b = TIFFGetB(image[i]);
      a = TIFFGetA(image[i]);

      switch (data_type)
	{
	case GEGL_FLOAT:
	  t1[j             ] = r / 255.0;
	  t1[j+plane_size  ] = g / 255.0;
	  t1[j+plane_size*2] = b / 255.0;
	  if (has_alpha)
	    t1[j+plane_size*3] = a / 255.0;
	  j++;
	  break; 
	case GEGL_U8:
	  t2[j             ] = r;
	  t2[j+plane_size  ] = g;
	  t2[j+plane_size*2] = b;
	  if (has_alpha)
	    t2[j+plane_size*3] = a;
	  j++;
	  break; 
	case GEGL_U16:
	  t3[j             ] = (r / 255.0) * 65535;
	  t3[j+plane_size  ] = (g / 255.0) * 65535;
	  t3[j+plane_size*2] = (b / 255.0) * 65535;
	  if (has_alpha)
	    t3[j+plane_size*3] = (a / 255.0) * 65535;
	  j++;
	  break; 
	case GEGL_U16_4:
	  t4[j             ] = (r / 255.0) * 4095;
	  t4[j+plane_size  ] = (g / 255.0) * 4095;
	  t4[j+plane_size*2] = (b / 255.0) * 4095;
	  if (has_alpha)
	    t4[j+plane_size*3] = (a / 255.0) * 4095;
	  j++;
	  break;
	default:
	 break;  
	}  
      
    }
    _TIFFfree(image);
    return image_data;
}
	      
int
main(int argc, 
     char *argv[])
{
  GeglColorModel  	*src_color_model[2];  /* 0 = src1, 1 = src2 */
  GeglImageBuffer 	*src_image_buffer[2]; /* 0 = src1, 1 = src2 */
  GtkWidget       	*src_window[2];
  GtkWidget       	*src_preview[2];
  guint          	src_width[2], src_height[2];
  GeglRect        	src_rect[2];
  int             	k;
  gint            	num_chans;
  TIFF                  *tif;   
  guchar                *image_data = NULL;
  guint16               samples_per_pixel;
  GeglChannelDataType   data_type = GEGL_FLOAT;
  gboolean              has_alpha = FALSE;

  gtk_init (&argc, &argv);

  /* find out what data type we will be using */
  if (argc > 3)
    {
      if (!strcmp (argv[3], "float"))
	{
	  data_type = GEGL_FLOAT;
	}
      else if (!strcmp (argv[3], "u8"))
	{
	  data_type = GEGL_U8;
	}
      else if (!strcmp (argv[3], "u16"))
	{
	  data_type = GEGL_U16;
	}
      else if (!strcmp (argv[3], "u16_4"))
	{
	  data_type = GEGL_U16_4;
	}
    }

  /* read in the 2 sources, src1 and src2 */ 
  for(k=0; k<=1; k++)
    {            
     
      has_alpha = FALSE;

      /* open the file */
      tif = TIFFOpen(argv[k+1], "r");

      /* get number of channels */
      TIFFGetFieldDefaulted (tif, 
	          TIFFTAG_SAMPLESPERPIXEL, 
		  &samples_per_pixel);

      /* heres a hack for has_alpha */
      if (samples_per_pixel == 2 || samples_per_pixel == 4)
        has_alpha = TRUE;

      if (has_alpha)
        printf ("%d has alpha\n", k);
      else
        printf ("%d does not have alpha\n", k);

      /* get width and height */
      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &src_width[k]);
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &src_height[k]);
      
      /* create the src GeglColorModel */
      src_color_model[k] = gegl_color_model_factory (GEGL_RGB, 
                                                    data_type, 
                                                    has_alpha);

      /* create the src GeglImageBuffer */
      src_image_buffer[k] = gegl_image_buffer_new (src_color_model[k], 
                                                   src_width[k], 
                                                   src_height[k]); 
        
      num_chans = gegl_color_model_num_channels(src_color_model[k]);

      /* get an image_data buffer we can pass to GeglImageBuffer */
      image_data = read_tiff_image_data (tif, 
	                                     src_width[k], 
                                         src_height[k], 
                                         num_chans, 
                                         data_type, 
                                         has_alpha);
      
      gegl_rect_set (&src_rect[k], 
                    0,0, 
                    src_width[k], src_height[k]);
      
      TIFFClose(tif); 

      /* pass the image data to the GeglImageBuffer */
      gegl_image_buffer_set_data(src_image_buffer[k], image_data);
      g_free(image_data);


      /* If srcs should be unpremultiplied before any ops are called,
	 you can do that here */
#if 0 
      if (has_alpha)
	unpremultiply_buffer(src_image_buffer[k], &src_rect[k]);
#endif 
    } 

  test_point_ops (src_image_buffer, src_width, src_height, src_rect); 
#if 0
  /* Test some of the GeglOps */  
  test_copy_op (src_image_buffer, src_width, src_height, src_rect);
  test_composite_ops (src_image_buffer, src_width, src_height, src_rect);
  test_point_ops (src_image_buffer, src_width, src_height, src_rect); 
#endif

  /* display the sources ie src1 and src2 */
  for (k = 0; k < 2; k++)
    {

      /* if unpremultiplied and should be premultiplied for display,
         you can do that here. */
#if 0 
      has_alpha = gegl_color_model_has_alpha (src_color_model[k]);
      if (has_alpha)
	premultiply_buffer(src_image_buffer[k], &src_rect[k]);
#endif 

      create_preview(&src_window[k], &src_preview[k], 
	             src_width[k], src_height[k], 
                     (k==0)?"src1":"src2");
      display_image(src_window[k], src_preview[k], 
	            src_image_buffer[k], src_rect[k],
	            data_type);
    }  

  for (k = 0; k < 2; k++)
    {
      gegl_object_unref(GEGL_OBJECT(src_image_buffer[k]));
      gegl_object_unref(GEGL_OBJECT(src_color_model[k]));
    }  

  gtk_main();

  return 0;
}

