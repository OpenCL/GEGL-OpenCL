#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>


#include "gegl-color.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u16.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-gray-u16.h"
#include "gegl-color-convert-to-rgb-op.h"
#include "gegl-color-convert-to-gray-op.h"
#include "gegl-copy-op.h"
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

void
point_ops_and_chains (void)
{
  /* This is a test of some simple point ops and op chains */    

    GeglColorModel *cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(TRUE));
    GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    {
      GeglImage *op, *op1, *op2; 
      GeglRect roi;
      GeglColor *c = gegl_color_new (cm);
      GeglChannelValue * chans = gegl_color_get_channel_values(c);

      /* Fill A with (.5.,.4,.3,.2) */
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

      /* Fill B with (.1.,.2,.3,.4) */
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

      /* 
	 Multiply A by B

	 A  B 
	  \/
	  op  (op is MultOp) 

	  Then we evaluate the op on destination image C
      */

      op = GEGL_IMAGE(gegl_mult_op_new (A,B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, C, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("mult A and B is: \n");
      print_result (C,0,0,2,2);

      /* 
	 This next tests the following chain of ops/images:

	 A  B 
	  \/
	  op1  (op1 is a CompositePremultOp with OVER mode) 
	   |
	  op2  (op2 is a TestOp, which is just division by 2) 

	  Then we evaluate the tree on destination image C
      */

      /* Note: B over A is given by (1-alphaB)*cA + cB */
      op1 = GEGL_IMAGE(gegl_composite_premult_op_new (A,B,COMPOSITE_OVER)); 
      op2 = GEGL_IMAGE(gegl_test_op_new (op1)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op2, C, &roi);
      gegl_object_destroy (GEGL_OBJECT(op2));
      gegl_object_destroy (GEGL_OBJECT(op1));
      printf("comp B over A, then divide by 2: \n");
      print_result (C,0,0,2,2);
    }
    gegl_object_destroy (GEGL_OBJECT(A));
    gegl_object_destroy (GEGL_OBJECT(B));
    gegl_object_destroy (GEGL_OBJECT(C));
    gegl_object_destroy (GEGL_OBJECT(cm));
}

void 
fillop_and_printop (void)
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

void
copy_ops (void)
{
    GeglImage *op; 
    GeglRect roi;
    GeglChannelValue * chans;

    GeglColorModel *A_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
    GeglColorModel *B_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
    GeglColorModel *C_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(FALSE));
    GeglColorModel *D_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(FALSE));
    GeglColorModel *E_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(FALSE));
    GeglColorModel *F_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(FALSE));

    GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(A_cm,2,2));
    GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(B_cm,2,2));
    GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(C_cm,2,2));
    GeglImage *D = GEGL_IMAGE(gegl_image_buffer_new(D_cm,2,2));
    GeglImage *E = GEGL_IMAGE(gegl_image_buffer_new(E_cm,2,2));
    GeglImage *F = GEGL_IMAGE(gegl_image_buffer_new(F_cm,2,2));

    GeglColor *A_color = gegl_color_new (A_cm);
    GeglColor *B_color = gegl_color_new (B_cm);
    GeglColor *C_color = gegl_color_new (C_cm);
    GeglColor *D_color = gegl_color_new (D_cm);
    GeglColor *E_color = gegl_color_new (E_cm);
    GeglColor *F_color = gegl_color_new (F_cm);

    chans = gegl_color_get_channel_values(A_color);
    chans[0].f = .1;
    chans[1].f = .2;
    chans[2].f = .3;

    /* Fill A with (.1,.2,.3) */
    op = GEGL_IMAGE(gegl_fill_op_new (A_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, A, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("A is: \n");
    print_result (A,0,0,2,2);

    chans = gegl_color_get_channel_values(B_color);
    chans[0].f = .4;
    chans[1].f = .5;
    chans[2].f = .6;

    /* Fill B with (.4.,.5,.6) */
    op =  GEGL_IMAGE(gegl_fill_op_new (B_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, B, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("B is: \n");
    print_result (B,0,0,2,2);

    /* Copy B to A, rgb_float->rgb_float */
    op = GEGL_IMAGE(gegl_copy_op_new (B)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, A, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("Copy B to A:\n");
    print_result (A,0,0,2,2);

    chans = gegl_color_get_channel_values(C_color);
    chans[0].u8 = 1;
    chans[1].u8 = 2;
    chans[2].u8 = 3;

    /* Fill C with (1,2,3)*/
    op = GEGL_IMAGE(gegl_fill_op_new (C_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, C, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("C is: \n");
    print_result (C,0,0,2,2);

    chans = gegl_color_get_channel_values(D_color);
    chans[0].u8 = 4;
    chans[1].u8 = 5;
    chans[2].u8 = 6;

    /* Fill D with (4,5,6) */
    op =  GEGL_IMAGE(gegl_fill_op_new (D_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, D, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("D is: \n");
    print_result (D,0,0,2,2);

    /* Copy D to C, rgb_u8->rgb_u8 */
    op = GEGL_IMAGE(gegl_copy_op_new (D)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, C, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("Copy D to C:\n");
    print_result (C,0,0,2,2);

    chans = gegl_color_get_channel_values(E_color);
    chans[0].u16 = 1000;
    chans[1].u16 = 2000;
    chans[2].u16 = 3000;

    /* Fill E with (1000,2000,3000)*/
    op = GEGL_IMAGE(gegl_fill_op_new (E_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, E, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("E is: \n");
    print_result (E,0,0,2,2);

    chans = gegl_color_get_channel_values(F_color);
    chans[0].u16 = 4000;
    chans[1].u16 = 5000;
    chans[2].u16 = 6000;

    /* Fill F with (4000,5000,6000) */
    op =  GEGL_IMAGE(gegl_fill_op_new (F_color)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, F, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("F is: \n");
    print_result (F,0,0,2,2);

    /* Copy F to E, rgb_u16->rgb_u16 */
    op = GEGL_IMAGE(gegl_copy_op_new (F)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, E, &roi);
    gegl_object_destroy (GEGL_OBJECT(op));
    printf("Copy F to E:\n");
    print_result (E,0,0,2,2);

    gegl_object_destroy (GEGL_OBJECT(A_cm));
    gegl_object_destroy (GEGL_OBJECT(B_cm));
    gegl_object_destroy (GEGL_OBJECT(C_cm));
    gegl_object_destroy (GEGL_OBJECT(D_cm));
    gegl_object_destroy (GEGL_OBJECT(A));
    gegl_object_destroy (GEGL_OBJECT(B));
    gegl_object_destroy (GEGL_OBJECT(C));
    gegl_object_destroy (GEGL_OBJECT(D));
    gegl_object_destroy (GEGL_OBJECT(A_color));
    gegl_object_destroy (GEGL_OBJECT(B_color));
    gegl_object_destroy (GEGL_OBJECT(C_color));
    gegl_object_destroy (GEGL_OBJECT(D_color));
}

void
color_convert_ops (void)
{
      GeglImage *op; 
      GeglRect roi;
      GeglChannelValue * chans;

    GeglColorModel *A_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_gray_float_new(FALSE));
    GeglColorModel *B_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
    GeglColorModel *C_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_gray_u8_new(FALSE));
    GeglColorModel *D_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(FALSE));
    GeglColorModel *E_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_gray_u16_new(FALSE));
    GeglColorModel *F_cm = 
      GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(FALSE));

    GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(A_cm,2,2));
    GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(B_cm,2,2));
    GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(C_cm,2,2));
    GeglImage *D = GEGL_IMAGE(gegl_image_buffer_new(D_cm,2,2));
    GeglImage *E = GEGL_IMAGE(gegl_image_buffer_new(E_cm,2,2));
    GeglImage *F = GEGL_IMAGE(gegl_image_buffer_new(F_cm,2,2));

    GeglColor *A_color = gegl_color_new (A_cm);
    GeglColor *B_color = gegl_color_new (B_cm);
    GeglColor *C_color = gegl_color_new (C_cm);
    GeglColor *D_color = gegl_color_new (D_cm);
    GeglColor *E_color = gegl_color_new (E_cm);
    GeglColor *F_color = gegl_color_new (F_cm);

      chans = gegl_color_get_channel_values(A_color);
      chans[0].f = .1;

      /* Fill A with .1 */
      op = GEGL_IMAGE(gegl_fill_op_new (A_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, A, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("A (gray_float) is: \n");
      print_result (A,0,0,2,2);

      chans = gegl_color_get_channel_values(B_color);
      chans[0].f = .2;
      chans[1].f = .3;
      chans[2].f = .4;

      /* Fill B with (.2.,.3,.4) */
      op =  GEGL_IMAGE(gegl_fill_op_new (B_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, B, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("B (rgb_float) is: \n");
      print_result (B,0,0,2,2);

      chans = gegl_color_get_channel_values(C_color);
      chans[0].u8 = 55;

      /* Convert B to A, rgb_float->gray_float (direct) */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, A, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("Convert B to A, rgb_float->gray_float (direct):\n");
      print_result (A,0,0,2,2);

      /* Fill C with 55 */
      op = GEGL_IMAGE(gegl_fill_op_new (C_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, C, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("C (gray_u8) is: \n");
      print_result (C,0,0,2,2);

      /* Convert B to C, rgb_float->gray_u8 (XYZ conversion)  */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, C, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("Convert B to C, rgb_float->gray_u8 (XYZ conversion):\n");
      print_result (C,0,0,2,2);

      chans = gegl_color_get_channel_values(D_color);
      chans[0].u8 = 10;
      chans[1].u8 = 20;
      chans[2].u8 = 30;

      /* Fill D with (10,20,30) */
      op = GEGL_IMAGE(gegl_fill_op_new (D_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, D, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("D (rgb_u8) is: \n");
      print_result (D,0,0,2,2);

      chans = gegl_color_get_channel_values(E_color);
      chans[0].u16 = 5000;

      /* Fill E with 5000 */
      op = GEGL_IMAGE(gegl_fill_op_new (E_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, E, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("E (gray_u16) is: \n");
      print_result (E,0,0,2,2);

      chans = gegl_color_get_channel_values(F_color);
      chans[0].u16 = 1000;
      chans[1].u16 = 2000;
      chans[2].u16 = 3000;

      /* Fill F with (1000,2000,3000) */
      op = GEGL_IMAGE(gegl_fill_op_new (F_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, F, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("F (rgb_u16) is: \n");
      print_result (F,0,0,2,2);

      /* Convert B to F, rgb_float->rgb_u16 (direct)  */
      op = GEGL_IMAGE(gegl_color_convert_to_rgb_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, F, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("Convert B to F, rgb_float->rgb_u16 (direct):\n");
      print_result (F,0,0,2,2);

      /* Convert B to E, rgb_float->gray_u16 (XYZ)  */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, E, &roi);
      gegl_object_destroy (GEGL_OBJECT(op));
      printf("Convert B to E, rgb_float->gray_u16 (XYZ):\n");
      print_result (E,0,0,2,2);

    gegl_object_destroy (GEGL_OBJECT(A_cm));
    gegl_object_destroy (GEGL_OBJECT(B_cm));
    gegl_object_destroy (GEGL_OBJECT(C_cm));
    gegl_object_destroy (GEGL_OBJECT(D_cm));
    gegl_object_destroy (GEGL_OBJECT(E_cm));
    gegl_object_destroy (GEGL_OBJECT(F_cm));
    gegl_object_destroy (GEGL_OBJECT(A));
    gegl_object_destroy (GEGL_OBJECT(B));
    gegl_object_destroy (GEGL_OBJECT(C));
    gegl_object_destroy (GEGL_OBJECT(D));
    gegl_object_destroy (GEGL_OBJECT(E));
    gegl_object_destroy (GEGL_OBJECT(F));
    gegl_object_destroy (GEGL_OBJECT(A_color));
    gegl_object_destroy (GEGL_OBJECT(B_color));
    gegl_object_destroy (GEGL_OBJECT(C_color));
    gegl_object_destroy (GEGL_OBJECT(D_color));
    gegl_object_destroy (GEGL_OBJECT(E_color));
    gegl_object_destroy (GEGL_OBJECT(F_color));
}

void
copy_chan_ops(void)
{

  /* This stuff is not up to date. Needs to be fixed. */

#if 0   /* This is a test of GeglCopyChanOp */ 

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
    op = GEGL_OP (gegl_copychan_op_new (dest, src, &r, &r, channel_indices));

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
#endif
}

int
main (int argc, char *argv[])
{  
  gtk_init (&argc, &argv);

  /* some tests */
  point_ops_and_chains();
  color_convert_ops();
  copy_ops();

  /*fillop_and_printop();*/
  /*copy_chans_ops();*/

  return 0;
}
