#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>

#include "gegl-color.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-image.h"
#include "gegl-image-buffer.h"
#include "gegl-op.h"
#include "gegl-fill-op.h"
#include "gegl-print-op.h"
#include "gegl-mult-op.h"
#include "gegl-premult-op.h"
#include "gegl-unpremult-op.h"
#include "gegl-composite-premult-op.h"
#include "gegl-composite-op.h"
#include "gegl-test-op.h"
#include "gegl-graphics-state.h"
#include "gegl-utils.h"

void
print_result (GeglImage *d,
          gint x,
	  gint y,
	  gint w,
	  gint h)
{
  GeglImage * op = GEGL_IMAGE(gegl_print_op_new()); 
  GeglRect roi;
  gegl_rect_set (&roi, x,y,w,h);
  gegl_image_get_pixels (op, d, &roi);
  gegl_object_destroy (GEGL_OBJECT(op)); 
  return;
}

int
main (int argc, char *argv[])
{  
  gtk_init (&argc, &argv);
  
#if 1 
    {
	GeglColorModel *cm = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(TRUE));
	GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
	GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
	GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));

        /* Fill the subrect at (0,0) with size 2 x 2  custom color */
        {
	  GeglImage *op, *op1, *op2; 
	  GeglRect roi;
	  GeglColor *c = gegl_color_new (cm);
	  GeglChannelValue * chans = gegl_color_get_channel_values(c);

	  /* Fill A with (.5.,.4,.3) */
	  chans[0].f = .5;
	  chans[1].f = .4;
	  chans[2].f = .3;
	  chans[3].f = .2;

	  op = GEGL_IMAGE(gegl_fill_op_new (c)); 
	  gegl_rect_set (&roi, 0,0,2,2);
	  gegl_image_get_pixels (op, A, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op));
	  printf("A is: \n");
	  print_result (A,0,0,2,2);

	  /* Fill B with (.1.,.2,.3) */
	  chans[0].f = .1;
	  chans[1].f = .2;
	  chans[2].f = .3;
	  chans[3].f = .4;

	  op =  GEGL_IMAGE(gegl_fill_op_new (c)); 
	  gegl_rect_set (&roi, 0,0,2,2);
	  gegl_image_get_pixels (op, B, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op));
	  printf("B is: \n");
	  print_result (B,0,0,2,2);


          /* multiply A by B and put in C, then print out */
	  op1 = GEGL_IMAGE(gegl_mult_op_new (A,B)); 
	  gegl_rect_set (&roi, 0,0,2,2);
	  gegl_image_get_pixels (op1, C, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op1));
	  printf("mult A and B is: \n");
	  print_result (C,0,0,2,2);

          /* composite B over A, divide by 2 (test_op), put in C */
	  /* B over A is (1-alphaB)*cA + cB */
	  op1 = GEGL_IMAGE(gegl_composite_premult_op_new (A,B,COMPOSITE_OVER)); 
	  op2 = GEGL_IMAGE(gegl_test_op_new (op1)); 
	  gegl_rect_set (&roi, 0,0,2,2);
	  gegl_image_get_pixels (op2, C, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op2));
	  gegl_object_destroy (GEGL_OBJECT(op1));
	  printf("comp B over A, divide by 2: \n");
	  print_result (C,0,0,2,2);

        }

	gegl_object_destroy (GEGL_OBJECT(A));
	gegl_object_destroy (GEGL_OBJECT(B));
	gegl_object_destroy (GEGL_OBJECT(C));
	gegl_object_destroy (GEGL_OBJECT(cm));
    }

#endif

#if 0   
  
  /* Small test of FillOp and the PrintOp */ 
  {
	GeglColorModel *cm = 
	  GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
	GeglImage *src = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
	GeglImage *dest = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));

        {
	  GeglImage *op; 
	  GeglRect roi;
	  GeglColor *c = gegl_color_new (cm);

	  /* Fill the 2 x 2 image with black */
          gegl_color_set_constant (c, COLOR_BLACK);
	  op =  GEGL_IMAGE(gegl_fill_op_new (c));
	  gegl_rect_set (&roi, 0,0,2,2);
	  gegl_image_get_pixels (op, dest, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op)); 

	  /* Fill the 1 x 1 subimage at (0,0) with blue */
          gegl_color_set_constant (c, COLOR_BLUE);
	  op =  GEGL_IMAGE(gegl_fill_op_new (c));
	  gegl_rect_set (&roi, 0,0,1,1);
	  gegl_image_get_pixels (op, dest, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op)); 
          
	  /* Fill the 1 x 1 subimage at (1,1) with red */
          gegl_color_set_constant (c, COLOR_RED);
	  op =  GEGL_IMAGE(gegl_fill_op_new (c));
	  gegl_rect_set (&roi, 1,1,1,1);
	  gegl_image_get_pixels (op, dest, &roi);
	  gegl_object_destroy (GEGL_OBJECT(op)); 

	  /* Print out the results */
	  print_result (dest,0,0,2,2);
        }

	gegl_object_destroy (GEGL_OBJECT(src));
	gegl_object_destroy (GEGL_OBJECT(dest));
	gegl_object_destroy (GEGL_OBJECT(cm));
  }
#endif


  /* 
    The rest of this stuff should be updated once gegl is 
    up to date. These examples are the old way now. 
  */

#if 0   /* This is a test of GeglCopyChanOp */ 
  {

	GeglColorModel *dest_cm = GEGL_COLOR_MODEL(
				gegl_color_model_rgb_float_new(FALSE));
	GeglImageBuffer *dest = gegl_image_buffer_new(dest_cm,5,5);

	GeglColorModel *src_cm = GEGL_COLOR_MODEL(
				gegl_color_model_rgb_float_new(FALSE));
	GeglImageBuffer *src = gegl_image_buffer_new(src_cm,5,5);

        /* Fill the whole 5 x 5 src image with RED */
        {
	  GeglOp *op;
	  GeglRect r;
	  GeglColor *c = gegl_color_new (dest_cm);
          gegl_color_set_constant (c, COLOR_RED);

	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_fill_op_new (src, &r, c));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
	  gegl_object_destroy (GEGL_OBJECT(c)); 
        }

        /* Fill the whole 5 x 5 dest image with GREEN */
        {
	  GeglOp *op;
	  GeglRect r;
	  GeglColor *c = gegl_color_new (dest_cm);
          gegl_color_set_constant (c, COLOR_GREEN);

	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_fill_op_new (dest, &r, c));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
	  gegl_object_destroy (GEGL_OBJECT(c)); 
        }

        /* Copy red src chan to green dest chan */
        {
	  GeglOp *op;
	  GeglRect r;
          gint channel_indices[4] = {0, 1, 0, -1};

	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_copy_chan_op_new (dest, src, &r, &r, channel_indices));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
        }

        /* Print out the image values for the src */
        {
          GeglOp *op;
	  GeglRect r;
	  gegl_rect_set (&r, 0,0,5,5);
	  op =  GEGL_OP (gegl_print_op_new (src, &r));
          gegl_op_apply (op);
	  gegl_object_destroy (GEGL_OBJECT(op)); 
	}

        /* Print out the image values for the dest*/
        {
          GeglOp *op;
	  GeglRect r;
	  gegl_rect_set (&r, 0,0,5,5);
	  op =  GEGL_OP (gegl_print_op_new (dest, &r));
          gegl_op_apply (op);
	  gegl_object_destroy (GEGL_OBJECT(op)); 
	}

        gegl_object_destroy (GEGL_OBJECT(src)); 
        gegl_object_destroy (GEGL_OBJECT(src_cm)); 
        gegl_object_destroy (GEGL_OBJECT(dest)); 
        gegl_object_destroy (GEGL_OBJECT(dest_cm)); 
  }
#endif
#if  0      /*This uses the GeglDrawable class */ 
  {

	GeglColorModel *cm = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(FALSE));
	GeglDrawable *d = GEGL_DRAWABLE(gegl_drawable_new(cm,"myDrawable",5,5));
	GeglGraphicsState *state = gegl_drawable_get_graphics_state (d);
	GeglRect r;

	gegl_rect_set (&r, 0,0,5,5);
        gegl_color_set_constant (state->fg_color, COLOR_RED);
        gegl_drawable_fill(d, &r); 

	gegl_rect_set (&r, 1,1,4,4);
        gegl_color_set_constant (state->fg_color, COLOR_GREEN);
        gegl_drawable_fill(d, &r); 


        /* Print out the image values using the 
	   print operator 
        */
        {
          GeglOp *op;
	  GeglRect r;
	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_print_op_new (gegl_drawable_get_image_buffer(d), &r));
          gegl_op_apply (op);
	  gegl_object_destroy (GEGL_OBJECT(op)); 

	}

	gegl_object_destroy (GEGL_OBJECT(d)); 
	gegl_object_destroy (GEGL_OBJECT(cm)); 

  }
#endif

#if 0  /*This illustrates a color conversion from rgb float to gray float*/ 
  {
    GeglColorModel *from_cm = GEGL_COLOR_MODEL(
                              gegl_color_model_rgb_float_new(FALSE));
    GeglImageBuffer *from_image = gegl_image_buffer_new(from_cm,2,2);
    GeglColorModel *to_cm = GEGL_COLOR_MODEL(
                            gegl_color_model_gray_float_new(FALSE));
    GeglImageBuffer *to_image = gegl_image_buffer_new(to_cm,2,2);


    GeglRect to_rect;
    GeglRect from_rect;

    gegl_rect_set (&to_rect, 0,0,2,2);
    gegl_rect_set (&from_rect, 0,0,2,2);

    /* Fill the 2 x 2 from image with GREEN */
    {
      GeglColor *c = gegl_color_new (from_cm);
      GeglOp *op;
      GeglRect fill_rect;
      gegl_rect_set (&fill_rect, 0,0,2,2);
      gegl_color_set_constant (c, COLOR_GREEN);
      op = GEGL_OP (gegl_fill_op_new (from_image, &fill_rect, c));

      gegl_op_apply (op);

      gegl_object_destroy (GEGL_OBJECT(op)); 
      gegl_object_destroy (GEGL_OBJECT(c)); 
    }

    /* Fill the 1 x 1 rect at (1,1) from image with RED */
    {
      GeglColor *c = gegl_color_new (from_cm);
      GeglOp *op;
      GeglRect fill_rect;
      gegl_rect_set (&fill_rect, 1,1,1,1);
      gegl_color_set_constant (c, COLOR_RED);
      op = GEGL_OP (gegl_fill_op_new (from_image, &fill_rect, c));

      gegl_op_apply (op);

      gegl_object_destroy (GEGL_OBJECT(op)); 
      gegl_object_destroy (GEGL_OBJECT(c)); 
    }

    /* Convert */
    {
      GeglOp* op = GEGL_OP (gegl_color_convert_to_gray_float_op_new( 
                     to_image, from_image, &to_rect, &from_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 
    }

    printf("The from image:\n");
    {
      GeglOp *op = GEGL_OP (gegl_print_op_new (from_image, &from_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 

    }
    printf("The to image:\n");
    {
      GeglOp *op = GEGL_OP (gegl_print_op_new (to_image, &to_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 
    }

    gegl_object_destroy (GEGL_OBJECT(from_image)); 
    gegl_object_destroy (GEGL_OBJECT(to_image)); 
    gegl_object_destroy (GEGL_OBJECT(from_cm)); 
    gegl_object_destroy (GEGL_OBJECT(to_cm)); 
  }
#endif

#if 0  /*This illustrates a color conversion using XYZ connection space */ 
  {
    GeglColorModel *from_cm = GEGL_COLOR_MODEL(
                              gegl_color_model_rgb_float_new(FALSE));
    GeglImageBuffer *from_image = gegl_image_buffer_new(from_cm,2,2);
    GeglColorModel *to_cm = GEGL_COLOR_MODEL(
                            gegl_color_model_gray_float_new(FALSE));
    GeglImageBuffer *to_image = gegl_image_buffer_new(to_cm,2,2);

    GeglRect to_rect;
    GeglRect from_rect;

    gegl_rect_set (&to_rect, 0,0,2,2);
    gegl_rect_set (&from_rect, 0,0,2,2);

    /* Fill the 2 x 2 from image with GREEN */
    {
      GeglColor *c = gegl_color_new (from_cm);
      GeglOp *op;
      GeglRect fill_rect;
      gegl_rect_set (&fill_rect, 0,0,2,2);
      gegl_color_set_constant (c, COLOR_GREEN);
      op = GEGL_OP (gegl_fill_op_new (from_image, &fill_rect, c));

      gegl_op_apply (op);

      gegl_object_destroy (GEGL_OBJECT(op)); 
      gegl_object_destroy (GEGL_OBJECT(c)); 
    }

    /* Fill the 1 x 1 rect at (1,1) from image with RED */
    {
      GeglColor *c = gegl_color_new (from_cm);
      GeglOp *op;
      GeglRect fill_rect;
      gegl_rect_set (&fill_rect, 1,1,1,1);
      gegl_color_set_constant (c, COLOR_RED);
      op = GEGL_OP (gegl_fill_op_new (from_image, &fill_rect, c));

      gegl_op_apply (op);

      gegl_object_destroy (GEGL_OBJECT(op)); 
      gegl_object_destroy (GEGL_OBJECT(c)); 
    }

    /* Convert */
    {
      GeglOp* op = GEGL_OP (gegl_color_convert_connection_op_new( 
                     to_image, from_image, &to_rect, &from_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 
    }


    printf("The from image:\n");
    {
      GeglOp *op = GEGL_OP (gegl_print_op_new (from_image, &from_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 

    }
    printf("The to image:\n");
    {
      GeglOp *op = GEGL_OP (gegl_print_op_new (to_image, &to_rect));
      gegl_op_apply (op);
      gegl_object_destroy (GEGL_OBJECT(op)); 
    }

    gegl_object_destroy (GEGL_OBJECT(from_image)); 
    gegl_object_destroy (GEGL_OBJECT(to_image)); 
    gegl_object_destroy (GEGL_OBJECT(from_cm)); 
    gegl_object_destroy (GEGL_OBJECT(to_cm)); 

  }
#endif

  return 0;
}
