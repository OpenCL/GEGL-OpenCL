#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>

#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-u16.h"
#include "gegl-color-model-rgb-u16_4.h"
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
#include "gegl-min-op.h"
#include "gegl-max-op.h"
#include "gegl-mult-op.h"
#include "gegl-diff-op.h"
#include "gegl-subtract-op.h"
#include "gegl-screen-op.h"
#include "gegl-dark-op.h"
#include "gegl-add-op.h"
#include "gegl-light-op.h"
#include "gegl-premult-op.h"
#include "gegl-types.h"


static GeglChannelDataType  DATA_TYPE;

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
              GeglRect rect)
{
  gint            w, h, num_chans;
  gfloat          **data_ptrs1;
  guint8          **data_ptrs2;
  guint16         **data_ptrs3;
  guint16         **data_ptrs4;
  int             i, j;
  guchar          *tmp;
  GeglRect        current_rect;
  num_chans = gegl_color_model_num_channels(
                gegl_image_buffer_color_model(image_buffer));
  data_ptrs1 = (gfloat**) g_malloc(sizeof(gfloat*) * num_chans);
  data_ptrs2 = (guint8**) g_malloc(sizeof(guint8*) * num_chans);
  data_ptrs3 = (guint16**) g_malloc(sizeof(guint16*) * num_chans);
  data_ptrs4 = (guint16**) g_malloc(sizeof(guint16*) * num_chans);
  h = rect.h;
  w = rect.w;
  tmp = g_new(char, 3*w);

  { 
    GeglImageIterator *iterator = gegl_image_iterator_new(image_buffer);
    gegl_image_iterator_request_rect(iterator, &rect);
    gegl_image_iterator_get_current_rect(iterator, &current_rect);

    for(i=0; i<h; i++)
      {
	switch (DATA_TYPE)
	  {
	  case FLOAT:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs1);

	    /* convert the data from float -> unsigned 8bit */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = data_ptrs1[0][j] * 255;
		tmp[1 + 3*j] = data_ptrs1[1][j] * 255;
		tmp[2 + 3*j] = data_ptrs1[2][j] * 255;
	      }
	    break;
	  case U8:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs2);

	    /* convert the data from float -> unsigned 8bit */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = data_ptrs2[0][j];
		tmp[1 + 3*j] = data_ptrs2[1][j];
		tmp[2 + 3*j] = data_ptrs2[2][j];
	      }
	    break;
	  case U16:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs3);

	    /* convert the data from float -> unsigned 8bit */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = data_ptrs3[0][j] * 255 / 65535.0;
		tmp[1 + 3*j] = data_ptrs3[1][j] * 255 / 65535.0;
		tmp[2 + 3*j] = data_ptrs3[2][j] * 255 / 65535.0;
	      }
	    break;
	  case U16_4:
	    gegl_image_iterator_get_scanline_data(iterator, 
		(guchar**)data_ptrs4);

	    /* convert the data from float -> unsigned 8bit */   
	    for(j=0; j<w; j++)
	      {
		tmp[0 + 3*j] = data_ptrs4[0][j] * 255 / 4095.0;
		tmp[1 + 3*j] = data_ptrs4[1][j] * 255 / 4095.0;
		tmp[2 + 3*j] = data_ptrs4[2][j] * 255 / 4095.0;
	      }
	    break;  
	  default:
	   break;  

	  }       
	gtk_preview_draw_row(GTK_PREVIEW(preview), tmp, 0, h-1-i, w);
	gegl_image_iterator_next_scanline(iterator);  
      }
    gegl_object_destroy (GEGL_OBJECT(iterator));
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
test_composite_ops( GeglImageBuffer ** src_image_buffer,
    guint * src_width,
    guint * src_height,
    GeglRect *src_rect )
{
  GeglRect               dest_rect;
  GeglImageBuffer       *dest_image_buffer;
  GeglColorModel        *dest_color_model;
  GeglOp 		*op;
  gint                   num_chans;
  gint                   i;
  gint                   width, height;
  GtkWidget       	*dest_window[6];
  GtkWidget       	*dest_preview[6];
  gchar	          	*mode_names[] = {"REPLACE", "OVER", 
                                         "IN", "OUT", 
                                         "ATOP", "XOR"};


  /* create the destination , same size as src 1 for now */
  width = src_width[0];
  height = src_height[0];
  dest_color_model = gegl_image_buffer_color_model (src_image_buffer[0]);  

  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                            width, height);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  gegl_rect_set (&dest_rect, 0, 0, width, height);

  for (i = 0; i< 6; i++) 
    {
      op = GEGL_OP(gegl_composite_op_new (dest_image_buffer, 
					  src_image_buffer[0], 
					  src_image_buffer[1], 
					  &src_rect[1], 
					  &dest_rect, 
					  &src_rect[0], 
					  i));
      gegl_op_apply (op);   
      gegl_object_destroy (GEGL_OBJECT(op));

#if 1 
      op = GEGL_OP(gegl_premult_op_new(dest_image_buffer, 
                                       dest_image_buffer,
                                       &dest_rect,
                                       &dest_rect));


      gegl_op_apply (op);   
      gegl_object_destroy (GEGL_OBJECT(op));
#endif

      /* display the destination */ 
      create_preview (&dest_window[i], &dest_preview[i], width, height, mode_names[i]);
      display_image (dest_window[i], dest_preview[i], dest_image_buffer,  dest_rect);
      gegl_object_destroy (GEGL_OBJECT(op));
    }

  gegl_object_destroy (GEGL_OBJECT (dest_image_buffer));
  gegl_object_destroy (GEGL_OBJECT (dest_color_model));
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

GeglOp *
point_op_factory (GeglImageBuffer * dest,
                  GeglImageBuffer **srcs,
                  GeglRect *dest_rect,
                  GeglRect *src_rect,
                  PointOpType type)
{
  switch (type)
    {
    case POINT_OP_MIN:
       return GEGL_OP(gegl_min_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_MAX:
       return GEGL_OP(gegl_max_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_MULT:
       return GEGL_OP(gegl_mult_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_SUBTRACT:
       return GEGL_OP(gegl_subtract_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_DIFF:
       return GEGL_OP(gegl_diff_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_SCREEN:
       return GEGL_OP(gegl_screen_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_DARK:
       return GEGL_OP(gegl_dark_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_LIGHT:
       return GEGL_OP(gegl_light_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
    case POINT_OP_PLUS:
       return GEGL_OP(gegl_add_op_new (dest, srcs[0], srcs[1], 
                    dest_rect, &src_rect[0], &src_rect[1]));
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
  GeglOp 		*op;
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
  dest_color_model = gegl_image_buffer_color_model (src_image_buffer[0]);  

  dest_image_buffer = gegl_image_buffer_new (dest_color_model, 
                                            width, height);
  num_chans = gegl_color_model_num_channels (dest_color_model);
  gegl_rect_set (&dest_rect, 0,0, width, height);

  for (i = 0; i < 9; i++) 
    {
      op = point_op_factory (dest_image_buffer, src_image_buffer, 
                            &dest_rect, src_rect, (PointOpType)i); 
      gegl_op_apply (op);   

      /* display the destination */ 
      create_preview (&dest_window[i], &dest_preview[i], width, height, point_op_names[i]);
      display_image (dest_window[i], dest_preview[i], dest_image_buffer, dest_rect);
      gegl_object_destroy (GEGL_OBJECT(op));
    }

  gegl_object_destroy (GEGL_OBJECT (dest_image_buffer));
  gegl_object_destroy (GEGL_OBJECT (dest_color_model));
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

  int             	i, j, k;
  gint            	num_chans;
  gfloat           	*t1=NULL;
  guint8           	*t2=NULL;
  guint16           	*t3=NULL;
  guint16           	*t4=NULL;

  /* stuff for reading a tiff */
  guchar          *image_data;
  TIFF            *tif;   
  uint32          *image;
  guchar          r,g,b,a; 
  gint            plane_size;

  gtk_init (&argc, &argv);

  /* find out what data type we will be using */
  if (argc > 3)
    {
      if (!strcmp (argv[3], "float"))
	{
	  DATA_TYPE = FLOAT;
	}
      else if (!strcmp (argv[3], "u8"))
	{
	  DATA_TYPE = U8;
	}
      else if (!strcmp (argv[3], "u16"))
	{
	  DATA_TYPE = U16;
	}
      else if (!strcmp (argv[3], "u16_4"))
	{
	  DATA_TYPE = U16_4;
	}
    }
  else
   DATA_TYPE = FLOAT;

  /* read in the 2 sources, src1 and src2 */ 

  for(k=0; k<=1; k++)
    {             
      /* open the file */
      tif = TIFFOpen(argv[k+1], "r");

      /* get width and height */
      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &src_width[k]);
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &src_height[k]);
      
      /* create the gegl image buffer */
      switch (DATA_TYPE)
	{
	case FLOAT:
	  src_color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(TRUE, TRUE));
	  break;
	case U8:
	  src_color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_u8_new(TRUE, TRUE));
	  break;
	case U16:
	  src_color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_u16_new(TRUE, TRUE));
	  break;
	case U16_4:
	  src_color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_u16_4_new(TRUE, TRUE));
	  break;
	default:
	  break;
	}
      src_image_buffer[k] = gegl_image_buffer_new(src_color_model[k], src_width[k], src_height[k]);
      num_chans = gegl_color_model_num_channels(src_color_model[k]);
      gegl_rect_set (&src_rect[k], 0,0, src_width[k], src_height[k]);
	      
      /* put the data from the image into image_data and make sure it is
      in the right format rrr ggg bbb */
      image = (uint32*) _TIFFmalloc(src_width[k] * src_height[k] * sizeof(uint32));
      image_data = (guchar*) g_malloc(sizeof(guchar) * src_width[k] * src_height[k] *
			      sizeof(float) * num_chans);
      
      switch (DATA_TYPE)
	{
	case FLOAT:
	  t1 = (float*) g_malloc(sizeof(float) * src_width[k] * src_height[k] * 4);
	  break;
	case U8:
	  t2 = (guint8*) g_malloc(sizeof(guint8) * src_width[k] * src_height[k] * 4);
	  break;
	case U16:
	  t3 = (guint16*) g_malloc(sizeof(guint16) * src_width[k] * src_height[k] * 4);
	  break;
	case U16_4:
	  t4 = (guint16*) g_malloc(sizeof(guint16) * src_width[k] * src_height[k] * 4);
	  break;
	default:
	  break; 
	}
      TIFFReadRGBAImage(tif, src_width[k], src_height[k], image, 0);
      j=0;
      plane_size = src_width[k] * src_height[k];
      for(i=0; i<plane_size; i++)
        {
          r = TIFFGetR(image[i]);
          g = TIFFGetG(image[i]);
          b = TIFFGetB(image[i]);
          a = TIFFGetA(image[i]);

	  switch (DATA_TYPE)
	    {
	    case FLOAT:
	      t1[j             ] = ((float)r) / 255.0;
	      t1[j+plane_size  ] = ((float)g) / 255.0;
	      t1[j+plane_size*2] = ((float)b) / 255.0;
	      t1[j+plane_size*3] = ((float)a) / 255.0;
	      j++;
	      break; 
	    case U8:
	      t2[j             ] = ((char)r);
	      t2[j+plane_size  ] = ((char)g);
	      t2[j+plane_size*2] = ((char)b);
	      t2[j+plane_size*3] = ((char)a);
	      j++;
	      break; 
	    case U16:
	      t3[j             ] = ((float)r) * 65535 / 255.0;
	      t3[j+plane_size  ] = ((float)g) * 65535 / 255.0;
	      t3[j+plane_size*2] = ((float)b) * 65535 / 255.0;
	      t3[j+plane_size*3] = ((float)a) * 65535 / 255.0;
	      j++;
	      break; 
	    case U16_4:
	      t4[j             ] = ((float)r) * 4095 / 255.0;
	      t4[j+plane_size  ] = ((float)g) * 4095 / 255.0;
	      t4[j+plane_size*2] = ((float)b) * 4095 / 255.0;
	      t4[j+plane_size*3] = ((float)a) * 4095 / 255.0;
	      j++;
	      break;
	    default:
	     break;  
	    }  
	  /*memcpy(img, &image[i], 4);*/
          
        }

      switch (DATA_TYPE)
	{
	case FLOAT:
	  memcpy(image_data, t1, src_width[k] * src_height[k] * sizeof(float) * num_chans);
	  break;
	case U8:
	  memcpy(image_data, t2, src_width[k] * src_height[k] * sizeof(guint8) * num_chans);
	  break;
	case U16:
	  memcpy(image_data, t3, src_width[k] * src_height[k] * sizeof(guint16) * num_chans);
	  break;
	case U16_4:
	  memcpy(image_data, t4, src_width[k] * src_height[k] * sizeof(guint16) * num_chans);
	  break;
	default:
	  break; 

	}
      _TIFFfree(image);
      TIFFClose(tif); 
      g_free(t1);
      g_free(t2);
      g_free(t3);
      g_free(t4);

      /* give the data to the gegl image buff */
      gegl_image_buffer_set_data(src_image_buffer[k], image_data);
      g_free(image_data);

      /* create the display window */
      create_preview(&src_window[k], &src_preview[k], src_width[k], src_height[k], 
                    (k==0)?"src1":"src2");
      display_image(src_window[k], src_preview[k], src_image_buffer[k], src_rect[k]);
    } 
     
  test_composite_ops (src_image_buffer, src_width, src_height, src_rect);
  /*test_point_ops (src_image_buffer, src_width, src_height, src_rect);*/ 

  for (k = 0; k < 2; k++)
    {
      gegl_object_destroy(GEGL_OBJECT(src_image_buffer[k]));
      gegl_object_destroy(GEGL_OBJECT(src_color_model[k]));
    }  
  gtk_main();

  return 0;
}






#if 0  
  /* create a buffer with all possible colors */
  {
	
     GeglColorModel *rgb_float_cm, *rgb_u8_cm, *gray_float_cm, *gray_u8_cm; 
     gfloat **xyz, *ss;
     gfloat **rgbfloat, *s, **grayfloat, *g;
     guint8 **rgbu8, *su8, **grayu8, *gu8;
     gint w=1;
     gint i, j, k, a;
     gfloat I, J, K, A, T;
     gfloat sum0=0, sum2=0, sum4=0, sum5=0, sum6=0;
     guint8 sum1=0, sum3=0, sum7=0;
     rgb_float_cm = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(TRUE, TRUE));
     rgb_u8_cm = GEGL_COLOR_MODEL(gegl_color_model_rgb_u8_new(TRUE, TRUE));
     gray_float_cm = GEGL_COLOR_MODEL(gegl_color_model_gray_float_new(TRUE, TRUE));
     gray_u8_cm = GEGL_COLOR_MODEL(gegl_color_model_gray_u8_new(TRUE, TRUE));
     T = 1.0 / 255.0;
     s = (gfloat*) g_malloc(sizeof(gfloat)*4);
     rgbfloat = (gfloat**) g_malloc(sizeof(gfloat*)*4);
     su8 = (guint8*) g_malloc(sizeof(guint8)*4);
     rgbu8 = (guint8**) g_malloc(sizeof(guint8*)*4);
     ss = (gfloat*) g_malloc(sizeof(gfloat)*4);
     xyz = (gfloat**) g_malloc(sizeof(gfloat*)*4);
     
     g = (gfloat*) g_malloc(sizeof(gfloat)*2);
     grayfloat = (gfloat**) g_malloc(sizeof(gfloat*)*2);
     gu8 = (guint8*) g_malloc(sizeof(guint8)*2);
     grayu8 = (guint8**) g_malloc(sizeof(guint8*)*2);

     for(i=0; i<2; i++){
       grayfloat[i] = &g[i];
       grayu8[i] = &gu8[i];
     }
     for(i=0; i<4;i++){
	rgbfloat[i] = &s[i];
  	rgbu8[i] = &su8[i];
	xyz[i] = &ss[i];
     }

     for(i=0; i<255; i+=100)
     for(j=0; j<255; j+=100)
     for(k=0; k<255; k+=100)
     for(a=0; a<255; a+=100){
       I = i * T;
       J = j * T;
       K = k * T;
       A = a * T;
             
       (rgbfloat[0][0]) = I;
       (rgbfloat[1][0]) = J;
       (rgbfloat[2][0]) = K;
       (rgbfloat[3][0]) = A;
       (grayfloat[0][0]) = I;
       (grayfloat[1][0]) = A; 
       (rgbu8[0][0]) = i;
       (rgbu8[1][0]) = j;
       (rgbu8[2][0]) = k;
       (rgbu8[3][0]) = a;
       (grayu8[0][0]) = i;
       (grayu8[1][0]) = a;


       /* rgb float -> xyz float -> rgb float */
       gegl_color_model_convert_to_xyz(rgb_float_cm, xyz, (guchar**)rgbfloat, w);
       gegl_color_model_convert_from_xyz(rgb_float_cm, (guchar**)rgbfloat, xyz, w);
       sum0 += (rgbfloat[0][0]-I)*(rgbfloat[0][0]-I) + (rgbfloat[1][0]-J)*(rgbfloat[1][0]-J) + 
               (rgbfloat[2][0]-K)*(rgbfloat[2][0]-K) + (rgbfloat[3][0]-A)*(rgbfloat[3][0]-A);

       /* rgb u8 -> xyz float -> rgb u8 */
       gegl_color_model_convert_to_xyz(rgb_u8_cm, xyz, (guchar**)rgbu8, w);
       gegl_color_model_convert_from_xyz(rgb_u8_cm, (guchar**)rgbu8, xyz, w);
       sum1 += (rgbu8[0][0]-i)*(rgbu8[0][0]-i) + (rgbu8[1][0]-j)*(rgbu8[1][0]-j) +
               (rgbu8[2][0]-k)*(rgbu8[2][0]-k) + (rgbu8[3][0]-a)*(rgbu8[3][0]-a);

       /* gray float -> xyz float -> gray float */
       gegl_color_model_convert_to_xyz(gray_float_cm, xyz, (guchar**)grayfloat, w);
       gegl_color_model_convert_from_xyz(gray_float_cm, (guchar**)grayfloat, xyz, w);
       sum2 += (grayfloat[0][0]-I)*(grayfloat[0][0]-I) + 
               (grayfloat[1][0]-A)*(grayfloat[1][0]-A);
       
       /* gray u8 -> xyz float -> gray u8 */
       gegl_color_model_convert_to_xyz(gray_u8_cm, xyz, (guchar**)grayu8, w);
       gegl_color_model_convert_from_xyz(gray_u8_cm, (guchar**)grayu8, xyz, w);
       sum3 += (grayu8[0][0]-i)*(grayu8[0][0]-i) + 
               (grayu8[1][0]-a)*(grayu8[1][0]-a);

       /* rgb float -> rgb u8 -> rgb float */
       (rgbfloat[0][0]) = I;
       (rgbfloat[1][0]) = J;
       (rgbfloat[2][0]) = K;
       (rgbfloat[3][0]) = A;
       gegl_color_model_convert_to_u8(rgb_float_cm, (guchar**)rgbu8, (guchar**)rgbfloat, w);
       gegl_color_model_convert_to_float(rgb_u8_cm, (guchar**)rgbfloat, (guchar**)rgbu8, w);
       sum4 += (rgbfloat[0][0]-I)*(rgbfloat[0][0]-I) + (rgbfloat[1][0]-J)*(rgbfloat[1][0]-J) +
               (rgbfloat[2][0]-K)*(rgbfloat[2][0]-K) + (rgbfloat[3][0]-A)*(rgbfloat[3][0]-A);

       /* gray float -> gray u8 -> gray float */
       (grayfloat[0][0]) = I;
       (grayfloat[1][0]) = A;
       gegl_color_model_convert_to_u8(gray_float_cm, (guchar**)grayu8, (guchar**)grayfloat, w);
       gegl_color_model_convert_to_float(gray_u8_cm, (guchar**)grayfloat, (guchar**)grayu8, w);
       sum5 += (grayfloat[0][0]-I)*(grayfloat[0][0]-I) + 
               (grayfloat[1][0]-A)*(grayfloat[1][0]-A);

       /* rgb float -> gray float -> rgb float */
       (rgbfloat[0][0]) = I;
       (rgbfloat[1][0]) = J;
       (rgbfloat[2][0]) = K;
       (rgbfloat[3][0]) = A;
       gegl_color_model_convert_to_gray(rgb_float_cm, (guchar**)grayfloat, (guchar**)rgbfloat, w);
       gegl_color_model_convert_to_rgb(gray_float_cm, (guchar**)rgbfloat, (guchar**)grayfloat, w);
       sum6 += (rgbfloat[0][0]-I)*(rgbfloat[0][0]-I) + (rgbfloat[1][0]-J)*(rgbfloat[1][0]-J) +
               (rgbfloat[2][0]-K)*(rgbfloat[2][0]-K) + (rgbfloat[3][0]-A)*(rgbfloat[3][0]-A);

       /* rgb u8 -> gray u8 -> rgb u8 */
       (rgbu8[0][0]) = i;
       (rgbu8[1][0]) = j;
       (rgbu8[2][0]) = k;
       (rgbu8[3][0]) = a;
       gegl_color_model_convert_to_gray(rgb_u8_cm, (guchar**)grayu8, (guchar**)rgbu8, w);
       gegl_color_model_convert_to_rgb(gray_u8_cm, (guchar**)rgbu8, (guchar**)grayu8, w);
       sum7 += (rgbu8[0][0]-I)*(rgbu8[0][0]-I) + (rgbu8[1][0]-J)*(rgbu8[1][0]-J) +
               (rgbu8[2][0]-K)*(rgbu8[2][0]-K) + (rgbu8[3][0]-A)*(rgbu8[3][0]-A);

     }
     printf("\n %.2f %d %.2f %d %.2f %.2f %.2f %d\n", sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7);

  }  
#endif
