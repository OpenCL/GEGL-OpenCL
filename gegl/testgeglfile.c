#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <tiffio.h>

#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-image-buffer.h"
#include "gegl-composite-op.h"
#include "gegl-utils.h"
#include "gegl-fill-op.h"
#include "gegl-color-model.h"
#include "gegl-color-model-rgb.h"

static void
create_preview(GtkWidget **window, GtkWidget **preview, int width, int height, gchar *name)
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
display_image(GtkWidget *window, GtkWidget *preview, 
                GeglImageBuffer* image_buffer,
                GeglRect current_rect)
{
  gint            w, h, num_chans;
  gfloat          **data_ptrs;
  int             i, j;
  guchar          tmp[3];

  num_chans = gegl_color_model_num_channels(gegl_image_buffer_color_model(image_buffer));
  data_ptrs = (gfloat**) g_malloc(sizeof(gfloat*) * num_chans);
  h = current_rect.h;
  w = current_rect.w;
  
  
  for(i=0; i<h; i++){
    gegl_image_buffer_get_scanline_data(image_buffer, (guchar**)data_ptrs);
    
    /* convert the data_ptrs */
    /* uchar -> float -> unsigned 8bit */   
    for(j=0; j<w; j++){
      tmp[0] = data_ptrs[0][j] * 255;
      tmp[1] = data_ptrs[1][j] * 255;
      tmp[2] = data_ptrs[2][j] * 255;

      gtk_preview_draw_row(GTK_PREVIEW(preview), tmp, j, i, 1);
    }       
    gegl_image_buffer_next_scanline(image_buffer);  
  }

  g_free(data_ptrs); 
  
  /* display the window */
  gtk_widget_show_all(window);
    
}       


int
main(int argc, char *argv[])
{
  GeglColorModel  	*color_model[2];
  GeglImageBuffer 	*image_buffer[2];
  GtkWidget       	*window[9];
  GtkWidget       	*preview[9];
  int             	i, j, k;
  GeglRect        	requested_rect;
  GeglRect        	current_rect[3];
  gint            	num_chans;
  float           	*t;
  uint32          	width[3], height[3];
  GeglOp 		*op;
  gchar	          	*win_name[] = {"REPLACE", "OVER", "IN",
				"OUT", "ATOP", "XOR", "PLUS"};
	 

  /* stuff for reading a image */
  guchar          *image_data;
  TIFF            *tif;   
  uint32          *image;
  guchar          img[4];

  gtk_init (&argc, &argv);
 
  /* read in the two images */ 
  for(k=0; k<2; k++){             
    /* open the file */
    tif = TIFFOpen(argv[k+1], "r");

    /* get width and height */
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width[k]);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height[k]);
    
    /* create the display window */
    create_preview(&window[k], &preview[k], width[k], height[k],
	(k==1)?"Background":"Foreground");

    /* create the gegl image buff */
    if(k)color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(FALSE, FALSE));
    else color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(TRUE, FALSE)); 
    image_buffer[k] = gegl_image_buffer_new(color_model[k], width[k], height[k]);
    num_chans = gegl_color_model_num_channels(color_model[k]);
	    
    /* put the data from the image into image_data and make sure it is
    in the right formate rrr ggg bbb */
    image = (uint32*) _TIFFmalloc(width[k] * height[k] * sizeof(uint32));
    image_data = (guchar*) g_malloc(sizeof(guchar) * width[k] * height[k] *
			    sizeof(float) * num_chans);
    t = (float*) g_malloc(sizeof(float) * width[k] * height[k] * 4);
    TIFFReadRGBAImage(tif, width[k], height[k], image, 0);
    j=0;
    for(i=0; i<width[k]*height[k]; i++){
      memcpy(img, &image[i], 4);
      t[j                     ] = ((float)img[3]) / 255.0;
      t[j+width[k]*height[k]  ] = ((float)img[2]) / 255.0;
      t[j+width[k]*height[k]*2] = ((float)img[1]) / 255.0;
      if(k) t[j+width[k]*height[k]*3] = ((float)img[0]) / 255.0;
      else t[j+width[k]*height[k]*3] = 0.5;  
      j++;  
    }

    memcpy(image_data, t, width[k] * height[k] * sizeof(float) * num_chans);
    _TIFFfree(image);
    TIFFClose(tif); 
    g_free(t);

    /* give the data to the gegl image buff */
    gegl_image_buffer_set_data(image_buffer[k], image_data);
    
    if(!k) g_free(image_data);

    /* init the rect */
    requested_rect.x = 0;           requested_rect.y = 0;
    requested_rect.w = width[k];    requested_rect.h = height[k];   
    
    gegl_image_buffer_request_rect(image_buffer[k], &requested_rect);
    gegl_image_buffer_get_current_rect(image_buffer[k], &current_rect[k]);
  } 
  display_image(window[0], preview[0], image_buffer[0], current_rect[0]);
  display_image(window[1], preview[1], image_buffer[1], current_rect[1]);
 
  requested_rect.x = 0;           requested_rect.y = 0;
  requested_rect.w = width[0];    requested_rect.h = height[0];
  gegl_image_buffer_request_rect(image_buffer[0], &requested_rect);
  gegl_image_buffer_get_current_rect(image_buffer[0], &current_rect[0]);
  
  requested_rect.x = 0;           requested_rect.y = 0;
  requested_rect.w = width[1];    requested_rect.h = height[1];
  gegl_image_buffer_request_rect(image_buffer[1], &requested_rect);
  gegl_image_buffer_get_current_rect(image_buffer[1], &current_rect[1]);
  
  /* test the comp ops :) */ 
  for(k=0; k<7; k++){
    op = GEGL_OP(gegl_composite_op_new (image_buffer[1], image_buffer[0], 
					    &(current_rect[1]), &(current_rect[0]),
					    k, FALSE));
    gegl_op_apply (op);   
    
    requested_rect.x = 0;           requested_rect.y = 0;
    requested_rect.w = width[1];    requested_rect.h = height[1];
    gegl_image_buffer_request_rect(image_buffer[1], &requested_rect);
    gegl_image_buffer_get_current_rect(image_buffer[1], &current_rect[1]);
    create_preview(&window[2+k], &preview[2+k], width[1], height[1], win_name[k]);  
    display_image(window[2+k], preview[2+k], image_buffer[1], current_rect[1]);
    gegl_image_buffer_set_data(image_buffer[1], image_data);

  }
  
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
  gtk_main();

  gtk_object_destroy (GTK_OBJECT(image_buffer)); 
  gtk_object_destroy (GTK_OBJECT(color_model)); 

  return 0;
}

