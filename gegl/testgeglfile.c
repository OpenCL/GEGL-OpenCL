#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <tiffio.h>

#include "gegl-color-model-rgb-float.h"
#include "gegl-image-buffer.h"
#include "gegl-composite-op.h"
#include "gegl-utils.h"
#include "gegl-fill-op.h"

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
    color_model[k] = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(TRUE, TRUE));
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
      t[j+width[k]*height[k]*3] = ((float)img[0]) / 255.0;
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
 
  /* test the convert op */
  /* create a buffer with all possible colors */
  /*{
    gfloat img_buf = (gfloat*) g_malloc(sizeof(gfloat)*4*255*255*255*255);
    gint i, j;
    for(i=0; i<4; i++)
    for(j=0; j<255; j++){
      img_buff[i*255+j] = ((gfloat)j)/255.0;
    }
    gegl_image_buffer_set_data(image_buffer[2], (guchar**)&img_buf);

    op = GEGL_OP(gegl_convert_op_new (image_buffer[2], image_buffer[3],
                                            &(current_rect[2]), &(current_rect[3]),
                                            ));
    gegl_op_apply (op);

     
  } */ 
  gtk_main();

  gtk_object_destroy (GTK_OBJECT(image_buffer)); 
  gtk_object_destroy (GTK_OBJECT(color_model)); 

  return 0;
}

