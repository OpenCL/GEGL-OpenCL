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
#include "gegl-copy-op.h"
#include "gegl-image-buffer.h"
#include "gegl-color-convert-to-rgb-op.h"
#include "gegl-color-convert-to-gray-op.h"
#include "gegl-add-op.h"
#include "gegl-composite-op.h"
#include "gegl-composite-premult-op.h"
#include "gegl-dark-op.h"
#include "gegl-diff-op.h"
#include "gegl-light-op.h"
#include "gegl-max-op.h"
#include "gegl-min-op.h"
#include "gegl-mult-op.h"
#include "gegl-screen-op.h"
#include "gegl-subtract-op.h"
#include "gegl-premult-op.h"
#include "gegl-test-op.h"
#include "gegl-unpremult-op.h"
#include "gegl-fill-op.h"
#include "gegl-print-op.h"
#include "gegl-utils.h"

/* global color models */
GeglColorModel * gRgba_u8 = NULL;
GeglColorModel * gRgba_u16 = NULL;
GeglColorModel * gRgba_float = NULL;
GeglColorModel * gGraya_u8 = NULL;
GeglColorModel * gGraya_u16 = NULL;
GeglColorModel * gGraya_float = NULL;
GeglColorModel * gRgb_u8 = NULL;
GeglColorModel * gRgb_u16 = NULL;
GeglColorModel * gRgb_float = NULL;
GeglColorModel * gGray_u8 = NULL;
GeglColorModel * gGray_u16 = NULL;
GeglColorModel * gGray_float = NULL;

void
init_color_models(void)
{
  gRgba_u8 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(TRUE));
  gRgba_u16 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(TRUE));
  gRgba_float = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(TRUE));
  gGraya_u8 = GEGL_COLOR_MODEL (gegl_color_model_gray_u8_new(TRUE));
  gGraya_u16 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_new(TRUE));
  gGraya_float = GEGL_COLOR_MODEL (gegl_color_model_gray_float_new(TRUE));
  gRgb_u8 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(FALSE));
  gRgb_u16 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(FALSE));
  gRgb_float = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
  gGray_u8 = GEGL_COLOR_MODEL (gegl_color_model_gray_u8_new(FALSE));
  gGray_u16 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_new(FALSE));
  gGray_float = GEGL_COLOR_MODEL (gegl_color_model_gray_float_new(FALSE));
}

void
free_color_models(void)
{
  gegl_object_unref (GEGL_OBJECT(gRgba_u8)); 
  gegl_object_unref (GEGL_OBJECT(gRgba_u16)); 
  gegl_object_unref (GEGL_OBJECT(gRgba_float)); 
  gegl_object_unref (GEGL_OBJECT(gGraya_u8)); 
  gegl_object_unref (GEGL_OBJECT(gGraya_u16)); 
  gegl_object_unref (GEGL_OBJECT(gGraya_float)); 
  gegl_object_unref (GEGL_OBJECT(gRgb_u8)); 
  gegl_object_unref (GEGL_OBJECT(gRgb_u16)); 
  gegl_object_unref (GEGL_OBJECT(gRgb_float)); 
  gegl_object_unref (GEGL_OBJECT(gGray_u8)); 
  gegl_object_unref (GEGL_OBJECT(gGray_u16)); 
  gegl_object_unref (GEGL_OBJECT(gGray_float)); 
}

void
print_image (GeglImage *d,
              gint x,
              gint y,
              gint w,
              gint h)
{
  GeglImage * op = GEGL_IMAGE(gegl_print_op_new()); 
  GeglRect roi;
  printf("Image %p, rect (%d,%d,%d,%d)\n", d,x,y,w,h); 
  gegl_rect_set (&roi, x,y,w,h);
  gegl_image_get_pixels (op, d, &roi);
  gegl_object_unref (GEGL_OBJECT(op)); 
  return;
}

void
print_image_all (GeglImage *image)
{
  gint w = gegl_image_get_width(image);
  gint h = gegl_image_get_height(image);
  print_image(image,0,0,w,h);
} 

void 
fill_image(GeglImage *image,
           GeglRect *rect,
           gfloat a, 
           gfloat b,
           gfloat c,
           gfloat d)
{
  GeglColorModel *cm = gegl_image_color_model(image);
  gint num_chan = gegl_color_model_num_channels(cm);
  GeglColor *color = gegl_color_new (cm);
  GeglChannelValue * chans = gegl_color_get_channel_values(color);
  GeglImage *op;

  /* Fill image with (a,b,c,d) */
  chans[0].f = a;
  if (num_chan > 1)
    chans[1].f = b;
  if (num_chan > 2)
    chans[2].f = c;
  if (num_chan > 3)
    chans[3].f = d;

  op = GEGL_IMAGE(gegl_fill_op_new (color)); 
  gegl_image_get_pixels (op, image, rect);

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(color));
} 

typedef enum
{
  PREMULT,
  TEST,
  UNPREMULT
}SingleSrcPointOpType;

GeglImage *
single_src_point_op_factory (SingleSrcPointOpType type,
                             GeglImage* s)
{
  switch (type)
    {
    case PREMULT:
       return GEGL_IMAGE(gegl_premult_op_new (s)); 
    case TEST:
       return GEGL_IMAGE(gegl_test_op_new (s)); 
    case UNPREMULT:
       return GEGL_IMAGE(gegl_unpremult_op_new (s)); 
    default:
       return NULL;
    }
}

typedef enum
{
  ADD,
  DARK,
  DIFF,
  LIGHT,
  MIN,
  MAX,
  MULT,
  SUBTRACT,
  SCREEN
}DualSrcPointOpType;

GeglImage *
dual_src_point_op_factory (DualSrcPointOpType type,
                           GeglImage* s1,
                           GeglImage* s2)
{
  switch (type)
    {
    case ADD:
       return GEGL_IMAGE(gegl_add_op_new (s1, s2)); 
    case DARK:
       return GEGL_IMAGE(gegl_dark_op_new (s1, s2)); 
    case DIFF:
       return GEGL_IMAGE(gegl_diff_op_new (s1, s2)); 
    case LIGHT:
       return GEGL_IMAGE(gegl_light_op_new (s1, s2)); 
    case MIN:
       return GEGL_IMAGE(gegl_min_op_new (s1, s2));
    case MAX:
       return GEGL_IMAGE(gegl_max_op_new (s1, s2));
    case MULT:
       return GEGL_IMAGE(gegl_mult_op_new (s1, s2)); 
    case SUBTRACT:
       return GEGL_IMAGE(gegl_subtract_op_new (s1, s2)); 
    case SCREEN:
       return GEGL_IMAGE(gegl_screen_op_new (s1, s2)); 
    default:
       return NULL;
    }
}

typedef enum
{
  COMP_ATOP,
  COMP_IN,
  COMP_OUT,
  COMP_OVER,
  COMP_REPLACE,
  COMP_XOR,
  COMP_PREMULT_ATOP,
  COMP_PREMULT_IN,
  COMP_PREMULT_OUT,
  COMP_PREMULT_OVER,
  COMP_PREMULT_REPLACE,
  COMP_PREMULT_XOR
}CompositeOpType;

GeglImage *
composite_op_factory (CompositeOpType type,
                      gchar **type_name,
                      GeglImage* s0,
                      GeglImage* s1)
{
  switch (type)
    {
    case COMP_ATOP:
       *type_name = g_strdup("atop");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_ATOP)); 
       
    case COMP_IN:
       *type_name = g_strdup("in");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_IN)); 
    case COMP_OUT:
       *type_name = g_strdup("out");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_OUT)); 
    case COMP_OVER:
       *type_name = g_strdup("over");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_OVER)); 
    case COMP_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_REPLACE)); 
    case COMP_XOR:
       *type_name = g_strdup("xor");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, COMPOSITE_XOR)); 
    case COMP_PREMULT_ATOP:
       *type_name = g_strdup("atop");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_ATOP)); 
    case COMP_PREMULT_IN:
       *type_name = g_strdup("in");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_IN)); 
    case COMP_PREMULT_OUT:
       *type_name = g_strdup("out");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_OUT)); 
    case COMP_PREMULT_OVER:
       *type_name = g_strdup("over");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_OVER)); 
    case COMP_PREMULT_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_REPLACE)); 
    case COMP_PREMULT_XOR:
       *type_name = g_strdup("xor");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, COMPOSITE_XOR)); 
    default:
       return NULL;
    }
}

void
test_single_src_point_op(SingleSrcPointOpType type, 
                         GeglColorModel *cm,
                         gint width, 
                         gint height)
{
  GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(cm,width,height));
  GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(cm,width,height));
  GeglImage *op = single_src_point_op_factory(type, A);
  GeglRect rect;

  gegl_rect_set(&rect, 0,0,width,height);

  /* Set up A */
  fill_image(A, &rect, .1, .2, .3, .4);
  printf("A is: ");
  print_image_all(A);

  /* Clear B */
  fill_image(B, &rect, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in B */
  gegl_image_get_pixels (op, B, &rect);
  printf("\n");
  printf("Result of %s: ", gegl_node_get_name(GEGL_NODE(op)));
  print_image_all(B);
  printf("\n");

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
}

void
test_dual_src_point_op(DualSrcPointOpType type, 
                       GeglColorModel *cm,
                       gint width, 
                       gint height)
{
  GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(cm,width,height));
  GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(cm,width,height));
  GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(cm,width,height));
  GeglImage *op = dual_src_point_op_factory(type, A,B);
  GeglRect rect;

  gegl_rect_set(&rect, 0,0,width,height);

  /* Set up A */
  fill_image(A, &rect, .1, .2, .3, .4);
  printf("A is: ");
  print_image_all(A);

  /* Set up B */
  fill_image(B, &rect, .5, .6, .7, .8);
  printf("\n");
  printf("B is: ");
  print_image_all(B);

  /* Clear C */
  fill_image(C, &rect, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in C */
  gegl_image_get_pixels (op, C, &rect);
  printf("\n");
  printf("Result of %s: ", gegl_node_get_name(GEGL_NODE(op)));
  print_image_all(C);
  printf("\n");

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(C));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
}

void
test_composite_op(CompositeOpType type, 
                  GeglColorModel *s0_cm,
                  GeglColorModel *s1_cm,
                  gint width, 
                  gint height)
{
  /* This will be s1 op s0 */
  gchar *type_name;
  GeglImage *A = GEGL_IMAGE(gegl_image_buffer_new(s0_cm,width,height));
  GeglImage *B = GEGL_IMAGE(gegl_image_buffer_new(s1_cm,width,height));
  GeglImage *C = GEGL_IMAGE(gegl_image_buffer_new(s0_cm,width,height));
  GeglImage *op = composite_op_factory(type, &type_name, A,B);
  GeglRect rect;

  gegl_rect_set(&rect, 0,0,width,height);

  /* Set up A */
  fill_image(A, &rect, .1, .2, .3, .4);
  printf("A is: ");
  print_image_all(A);

  /* Set up B */
  fill_image(B, &rect, .5, .6, .7, .8);
  printf("\n");
  printf("B is: ");
  print_image_all(B);

  /* Clear C */
  fill_image(C, &rect, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in C */
  gegl_image_get_pixels (op, C, &rect);
  printf("\n");
  printf("Result of %s %s: ", gegl_node_get_name(GEGL_NODE(op)), type_name);
  print_image_all(C);
  printf("\n");

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(C));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
  g_free(type_name);
}


void
point_ops_and_chains (void)
{
  /* This is a test of some simple point ops and op chains */    

    GeglColorModel *cm = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(TRUE));
    GeglImage *A;
    GeglImage *B;
    GeglImage *C;

    A = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    B = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    C = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
    {
      GeglImage *op1, *op2; 
      GeglRect roi;

      gegl_rect_set(&roi,0,0,2,2);

      /* Set up A */
      fill_image(A, &roi, .1, .2, .3, .4);
      printf("A is: ");
      print_image_all(A);

      /* Set up B */
      fill_image(B, &roi, .5, .6, .7, .8);
      printf("\n");
      printf("B is: ");
      print_image_all(B);

      /* Clear C */
      fill_image(C, &roi, 0.0, 0.0, 0.0, 0.0);

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
      printf("\n");
      printf("Result of Test(CompPremult(A,B,OVER)): ");
      print_image_all(C);
      printf("\n");

      gegl_object_unref (GEGL_OBJECT(op2));
      gegl_object_unref (GEGL_OBJECT(op1));

    }
    gegl_object_unref (GEGL_OBJECT(A));
    gegl_object_unref (GEGL_OBJECT(B));
    gegl_object_unref (GEGL_OBJECT(C));
    gegl_object_unref (GEGL_OBJECT(cm));
}

#if 0
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
    gegl_object_unref (GEGL_OBJECT(op)); 

    /* Fill the 1 x 1 subimage at (0,0) with blue */
    gegl_color_set_constant (c, COLOR_BLUE);
    op =  GEGL_IMAGE(gegl_fill_op_new (c));
    gegl_rect_set (&roi, 0,0,1,1);
    gegl_image_get_pixels (op, dest, &roi);
    gegl_object_unref (GEGL_OBJECT(op)); 

    /* Fill the 1 x 1 subimage at (1,1) with red */
    gegl_color_set_constant (c, COLOR_RED);
    op =  GEGL_IMAGE(gegl_fill_op_new (c));
    gegl_rect_set (&roi, 1,1,1,1);
    gegl_image_get_pixels (op, dest, &roi);
    gegl_object_unref (GEGL_OBJECT(op)); 

    /* Print out the results */
    print_result (dest,0,0,2,2);

    gegl_object_unref (GEGL_OBJECT(c));
  }

  gegl_object_unref (GEGL_OBJECT(src));
  gegl_object_unref (GEGL_OBJECT(dest));
  gegl_object_unref (GEGL_OBJECT(cm));
}
#endif

#if 0
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
    gegl_object_unref (GEGL_OBJECT(op));
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
    gegl_object_unref (GEGL_OBJECT(op));
    printf("B is: \n");
    print_result (B,0,0,2,2);

    /* Copy B to A, rgb_float->rgb_float */
    op = GEGL_IMAGE(gegl_copy_op_new (B)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, A, &roi);
    gegl_object_unref (GEGL_OBJECT(op));
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
    gegl_object_unref (GEGL_OBJECT(op));
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
    gegl_object_unref (GEGL_OBJECT(op));
    printf("D is: \n");
    print_result (D,0,0,2,2);

    /* Copy D to C, rgb_u8->rgb_u8 */
    op = GEGL_IMAGE(gegl_copy_op_new (D)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, C, &roi);
    gegl_object_unref (GEGL_OBJECT(op));
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
    gegl_object_unref (GEGL_OBJECT(op));
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
    gegl_object_unref (GEGL_OBJECT(op));
    printf("F is: \n");
    print_result (F,0,0,2,2);

    /* Copy F to E, rgb_u16->rgb_u16 */
    op = GEGL_IMAGE(gegl_copy_op_new (F)); 
    gegl_rect_set (&roi, 0,0,2,2);
    gegl_image_get_pixels (op, E, &roi);
    gegl_object_unref (GEGL_OBJECT(op));
    printf("Copy F to E:\n");
    print_result (E,0,0,2,2);

    gegl_object_unref (GEGL_OBJECT(A_cm));
    gegl_object_unref (GEGL_OBJECT(B_cm));
    gegl_object_unref (GEGL_OBJECT(C_cm));
    gegl_object_unref (GEGL_OBJECT(D_cm));
    gegl_object_unref (GEGL_OBJECT(E_cm));
    gegl_object_unref (GEGL_OBJECT(F_cm));
    gegl_object_unref (GEGL_OBJECT(A));
    gegl_object_unref (GEGL_OBJECT(B));
    gegl_object_unref (GEGL_OBJECT(C));
    gegl_object_unref (GEGL_OBJECT(D));
    gegl_object_unref (GEGL_OBJECT(E));
    gegl_object_unref (GEGL_OBJECT(F));
    gegl_object_unref (GEGL_OBJECT(A_color));
    gegl_object_unref (GEGL_OBJECT(B_color));
    gegl_object_unref (GEGL_OBJECT(C_color));
    gegl_object_unref (GEGL_OBJECT(D_color));
    gegl_object_unref (GEGL_OBJECT(E_color));
    gegl_object_unref (GEGL_OBJECT(F_color));
}
#endif

#if 0
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
      gegl_object_unref (GEGL_OBJECT(op));
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
      gegl_object_unref (GEGL_OBJECT(op));
      printf("B (rgb_float) is: \n");
      print_result (B,0,0,2,2);

      chans = gegl_color_get_channel_values(C_color);
      chans[0].u8 = 55;

      /* Convert B to A, rgb_float->gray_float (direct) */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, A, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
      printf("Convert B to A, rgb_float->gray_float (direct):\n");
      print_result (A,0,0,2,2);

      /* Fill C with 55 */
      op = GEGL_IMAGE(gegl_fill_op_new (C_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, C, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
      printf("C (gray_u8) is: \n");
      print_result (C,0,0,2,2);

      /* Convert B to C, rgb_float->gray_u8 (XYZ conversion)  */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, C, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
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
      gegl_object_unref (GEGL_OBJECT(op));
      printf("D (rgb_u8) is: \n");
      print_result (D,0,0,2,2);

      chans = gegl_color_get_channel_values(E_color);
      chans[0].u16 = 5000;

      /* Fill E with 5000 */
      op = GEGL_IMAGE(gegl_fill_op_new (E_color)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, E, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
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
      gegl_object_unref (GEGL_OBJECT(op));
      printf("F (rgb_u16) is: \n");
      print_result (F,0,0,2,2);

      /* Convert B to F, rgb_float->rgb_u16 (direct)  */
      op = GEGL_IMAGE(gegl_color_convert_to_rgb_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, F, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
      printf("Convert B to F, rgb_float->rgb_u16 (direct):\n");
      print_result (F,0,0,2,2);

      /* Convert B to E, rgb_float->gray_u16 (XYZ)  */
      op = GEGL_IMAGE(gegl_color_convert_to_gray_op_new (B)); 
      gegl_rect_set (&roi, 0,0,2,2);
      gegl_image_get_pixels (op, E, &roi);
      gegl_object_unref (GEGL_OBJECT(op));
      printf("Convert B to E, rgb_float->gray_u16 (XYZ):\n");
      print_result (E,0,0,2,2);

    gegl_object_unref (GEGL_OBJECT(A_cm));
    gegl_object_unref (GEGL_OBJECT(B_cm));
    gegl_object_unref (GEGL_OBJECT(C_cm));
    gegl_object_unref (GEGL_OBJECT(D_cm));
    gegl_object_unref (GEGL_OBJECT(E_cm));
    gegl_object_unref (GEGL_OBJECT(F_cm));
    gegl_object_unref (GEGL_OBJECT(A));
    gegl_object_unref (GEGL_OBJECT(B));
    gegl_object_unref (GEGL_OBJECT(C));
    gegl_object_unref (GEGL_OBJECT(D));
    gegl_object_unref (GEGL_OBJECT(E));
    gegl_object_unref (GEGL_OBJECT(F));
    gegl_object_unref (GEGL_OBJECT(A_color));
    gegl_object_unref (GEGL_OBJECT(B_color));
    gegl_object_unref (GEGL_OBJECT(C_color));
    gegl_object_unref (GEGL_OBJECT(D_color));
    gegl_object_unref (GEGL_OBJECT(E_color));
    gegl_object_unref (GEGL_OBJECT(F_color));
}
#endif

#if 0
void
copy_chan_ops(void)
{

  /* This stuff is not up to date (1-22-01). Needs to be fixed. */

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

    gegl_object_unref (GEGL_OBJECT(op)); 
    gegl_object_unref (GEGL_OBJECT(c)); 
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

    gegl_object_unref (GEGL_OBJECT(op)); 
    gegl_object_unref (GEGL_OBJECT(c)); 
  }

  /* Copy red src chan to green dest chan */
  {
    GeglOp *op;
    GeglRect r;
    gint channel_indices[4] = {0, 1, 0, -1};

    gegl_rect_set (&r, 0,0,5,5);
    op = GEGL_OP (gegl_copychan_op_new (dest, src, &r, &r, channel_indices));

    gegl_op_apply (op);

    gegl_object_unref (GEGL_OBJECT(op)); 
  }

  /* Print out the image values for the src */
  {
    GeglOp *op;
    GeglRect r;
    gegl_rect_set (&r, 0,0,5,5);
    op =  GEGL_OP (gegl_print_op_new (src, &r));
    gegl_op_apply (op);
    gegl_object_unref (GEGL_OBJECT(op)); 
  }

  /* Print out the image values for the dest*/
  {
    GeglOp *op;
    GeglRect r;
    gegl_rect_set (&r, 0,0,5,5);
    op =  GEGL_OP (gegl_print_op_new (dest, &r));
    gegl_op_apply (op);
    gegl_object_unref (GEGL_OBJECT(op)); 
  }

  gegl_object_unref (GEGL_OBJECT(src)); 
  gegl_object_unref (GEGL_OBJECT(src_cm)); 
  gegl_object_unref (GEGL_OBJECT(dest)); 
  gegl_object_unref (GEGL_OBJECT(dest_cm)); 
#endif
}
#endif



int
main (int argc, char *argv[])
{  
  /*Makes efence work*/
  gint w = 2;
  gint h = 2;

  int *p = malloc(sizeof(int) * 2); 
  p = NULL;

  gtk_init (&argc, &argv);

  init_color_models();

  test_dual_src_point_op (ADD,gRgb_float,w,h);
  test_dual_src_point_op (DARK,gRgb_float,w,h);
  test_dual_src_point_op (DIFF,gRgb_float,w,h);
  test_dual_src_point_op (LIGHT,gRgb_float,w,h);
  test_dual_src_point_op (MIN,gRgb_float,w,h);
  test_dual_src_point_op (MAX,gRgb_float,w,h);
  test_dual_src_point_op (MULT,gRgb_float,w,h);
  test_dual_src_point_op (SUBTRACT,gRgb_float,w,h);
  test_dual_src_point_op (SCREEN,gRgb_float,w,h);

  test_single_src_point_op (PREMULT,gRgba_float,w,h);
  test_single_src_point_op (TEST,gRgba_float,w,h);
  test_single_src_point_op (UNPREMULT,gRgba_float,w,h);

  test_composite_op (COMP_ATOP,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_IN,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_OUT,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_OVER,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_REPLACE,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_XOR,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_ATOP,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_IN,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_OUT,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_OVER,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_REPLACE,gRgba_float,gRgba_float,w,h);
  test_composite_op (COMP_PREMULT_XOR,gRgba_float,gRgba_float,w,h);

  free_color_models();

#if 0
  /* some tests */
  fillop_and_printop();
  point_ops_and_chains();
  color_convert_ops();
  copy_ops();
  copy_chans_ops();
#endif

  return 0;
}
