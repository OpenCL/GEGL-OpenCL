#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>

#include "gegl-add-op.h"
#include "gegl-cache.h"
#include "gegl-color.h"
#include "gegl-color-convert-op.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u16.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-gray-u16.h"
#include "gegl-composite-op.h"
#include "gegl-composite-premult-op.h"
#include "gegl-copy-op.h"
#include "gegl-dark-op.h"
#include "gegl-diff-op.h"
#include "gegl-fill-op.h"
#include "gegl-image-buffer.h"
#include "gegl-light-op.h"
#include "gegl-max-op.h"
#include "gegl-min-op.h"
#include "gegl-mult-op.h"
#include "gegl-premult-op.h"
#include "gegl-print-op.h"
#include "gegl-screen-op.h"
#include "gegl-subtract-op.h"
#include "gegl-tile-image-manager.h"
#include "gegl-tile.h"
#include "gegl-test-op.h"
#include "gegl-unpremult-op.h"
#include "gegl-utils.h"

GeglImage * 
fill_op_new(GeglColorModel *cm,
            gfloat a, 
            gfloat b,
            gfloat c,
            gfloat d)
{
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
  gegl_object_unref (GEGL_OBJECT(color));

  return op;
} 

void
print_image (GeglImage *d,
              gint x,
              gint y,
              gint w,
              gint h)
{
  GeglImage * print = GEGL_IMAGE(gegl_print_op_new(d)); 
  GeglRect roi;
  gegl_rect_set (&roi, x,y,w,h);

  printf("Image %x, rect (%d,%d,%d,%d)\n", (gint)d,x,y,w,h); 
  gegl_image_get_pixels (print, NULL, &roi);
  gegl_object_unref (GEGL_OBJECT(print));
  return;
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
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_ATOP)); 
    case COMP_IN:
       *type_name = g_strdup("in");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_IN)); 
    case COMP_OUT:
       *type_name = g_strdup("out");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_OUT)); 
    case COMP_OVER:
       *type_name = g_strdup("over");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_OVER)); 
    case COMP_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_REPLACE)); 
    case COMP_XOR:
       *type_name = g_strdup("xor");
       return GEGL_IMAGE(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_XOR)); 
    case COMP_PREMULT_ATOP:
       *type_name = g_strdup("atop");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_ATOP)); 
    case COMP_PREMULT_IN:
       *type_name = g_strdup("in");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_IN)); 
    case COMP_PREMULT_OUT:
       *type_name = g_strdup("out");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_OUT)); 
    case COMP_PREMULT_OVER:
       *type_name = g_strdup("over");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_OVER)); 
    case COMP_PREMULT_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_REPLACE)); 
    case COMP_PREMULT_XOR:
       *type_name = g_strdup("xor");
       return GEGL_IMAGE(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_XOR)); 
    default:
       return NULL;
    }
}

void
fill_image(GeglImageBuffer *image_buffer,
            gfloat a, 
            gfloat b,
            gfloat c,
            gfloat d)
{
  GeglColorModel *cm = gegl_image_color_model(GEGL_IMAGE(image_buffer));
  gint num_chan = gegl_color_model_num_channels(cm);
  GeglColor *color = gegl_color_new (cm);
  GeglChannelValue * chans = gegl_color_get_channel_values(color);
  GeglImage *fill = GEGL_IMAGE(gegl_fill_op_new (color)); 
  gint width = gegl_image_buffer_get_width(image_buffer);
  gint height = gegl_image_buffer_get_height(image_buffer);
  GeglRect roi;
  gegl_rect_set(&roi, 0,0,width,height);

  /* Fill image with (a,b,c,d) */
  chans[0].f = a;
  if (num_chan > 1)
    chans[1].f = b;
  if (num_chan > 2)
    chans[2].f = c;
  if (num_chan > 3)
    chans[3].f = d;

  gegl_image_get_pixels (fill,GEGL_IMAGE(image_buffer),&roi);

  gegl_object_unref (GEGL_OBJECT(color));
  gegl_object_unref(GEGL_OBJECT(fill));
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
  fill_image(GEGL_IMAGE_BUFFER(A), .1, .2, .3, .4);
  printf("A is: ");
  print_image(A, 0,0,width,height);

  /* Clear B */
  fill_image(GEGL_IMAGE_BUFFER(B), 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in B */
  gegl_image_get_pixels (op, B, &rect);
  printf("\n");
  printf("Result of %s: ", gegl_node_get_name(GEGL_NODE(op)));
  print_image(B, 0,0,width,height);
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
  fill_image(GEGL_IMAGE_BUFFER(A), .1, .2, .3, .4);
  printf("A is: ");
  print_image(A, 0,0,width,height);
  /* Set up B */
  fill_image(GEGL_IMAGE_BUFFER(B), .5, .6, .7, .8);
  printf("\n");
  printf("B is: ");
  print_image(B, 0,0,width,height);

  /* Clear C */
  fill_image(GEGL_IMAGE_BUFFER(C), 0.0, 0.0, 0.0, 0.0);


  /* Do the operation, put in C */
  gegl_image_get_pixels (op, C, &rect);
  printf("\n");
  printf("Result of %s: ", gegl_node_get_name(GEGL_NODE(op)));
  print_image(C, 0,0,width,height);
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
  fill_image(GEGL_IMAGE_BUFFER(A), .1, .2, .3, .4);
  printf("A is: ");
  print_image(A, 0,0,width,height);

  /* Set up B */
  fill_image(GEGL_IMAGE_BUFFER(B), .5, .6, .7, .8);
  printf("\n");
  printf("B is: ");
  print_image(B, 0,0,width,height);

  /* Clear C */
  fill_image(GEGL_IMAGE_BUFFER(C), 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in C */
  gegl_image_get_pixels (op, C, &rect);
  printf("\n");
  printf("Result of %s %s: ", gegl_node_get_name(GEGL_NODE(op)), type_name);
  print_image(C, 0,0,width,height);
  printf("\n");

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(C));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
  g_free(type_name);
}


void
fill_buffer_and_print (void)
{
  GeglImage *fill; 
  GeglImage *print; 
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglImage *buffer = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /*

    fill -> buffer 

    print
     |
    buffer

  */ 

  fill = fill_op_new(cm, .1, .2, .3, .4);

  gegl_image_get_pixels (fill,buffer,&roi);

  /* print it */
  print = GEGL_IMAGE(gegl_print_op_new(buffer)); 
  gegl_image_get_pixels (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(fill));
  gegl_object_unref (GEGL_OBJECT(buffer));
}

void
simple_fill(void)
{
  GeglImage *fill; 
  GeglImage *print; 
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /*
    print
      |
    fill  

  */ 

  fill = fill_op_new(cm, .1, .2, .3, .4);
  print = GEGL_IMAGE(gegl_print_op_new(fill)); 

  gegl_image_get_pixels (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(fill));
}

void
linear_chain_test(void)
{
  GeglImage *fill;
  GeglImage *test1, *test2;
  GeglImage *print;
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglImage *buffer = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /*
    test2 -> buffer 
      |   
    test1
      |
    fill  

  */ 

  fill = fill_op_new(cm, .1, .2, .3, .4);

  test1 = GEGL_IMAGE(gegl_test_op_new(fill)); 
  test2 = GEGL_IMAGE(gegl_test_op_new(test1)); 

  gegl_image_get_pixels (test2,buffer,&roi);

  /* print it */
  print = GEGL_IMAGE(gegl_print_op_new(buffer)); 
  gegl_image_get_pixels (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(fill));
  gegl_object_unref (GEGL_OBJECT(test1));
  gegl_object_unref (GEGL_OBJECT(test2));
  gegl_object_unref (GEGL_OBJECT(buffer));
}

void
simple_tree(void)
{
  GeglImage *fill1, *fill2;
  GeglImage *add;
  GeglImage *print;
  GeglImage *test;
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglImage *buffer = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /*

        add -> buffer
        /  \ 
    test   fill2 
      |
    fill1  

  */ 

  fill1 = fill_op_new(cm, .1, .2, .3, .4);
  fill2 = fill_op_new(cm, .5, .6, .7, .8);

  test = GEGL_IMAGE(gegl_test_op_new(fill1)); 

  add = GEGL_IMAGE(gegl_add_op_new(fill2,test)); 

  gegl_image_get_pixels (add,buffer,&roi);

  /* print buffer */
  print = GEGL_IMAGE(gegl_print_op_new(buffer)); 
  gegl_image_get_pixels (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(fill1));
  gegl_object_unref (GEGL_OBJECT(test));
  gegl_object_unref (GEGL_OBJECT(fill2));
  gegl_object_unref (GEGL_OBJECT(add));
  gegl_object_unref (GEGL_OBJECT(buffer));
}

void
color_convert_ops (void)
{
  GeglColorModel *cm1 =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglColorModel *cm2 =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT);

  GeglImage *fill_op; 
  GeglImage *print_op; 
  GeglImage *convert_op; 
  GeglImage *buffer1 = GEGL_IMAGE(gegl_image_buffer_new(cm1,2,2));
  GeglImage *buffer2 = GEGL_IMAGE(gegl_image_buffer_new(cm2,2,2));
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /* fill buffer1 */ 
  fill_op = fill_op_new(cm1, .1, .2, .3, .4);
  gegl_image_get_pixels (fill_op,buffer1,&roi);

  /* color convert, put in buffer2 */
  convert_op = GEGL_IMAGE(gegl_color_convert_op_new(buffer1, cm2));
  gegl_image_get_pixels (convert_op,buffer2,&roi);

  /* print buffer2 */
  print_op = GEGL_IMAGE(gegl_print_op_new(buffer2));
  gegl_image_get_pixels (print_op,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print_op));
  gegl_object_unref (GEGL_OBJECT(convert_op));
  gegl_object_unref (GEGL_OBJECT(fill_op));
  gegl_object_unref (GEGL_OBJECT(buffer2));
  gegl_object_unref (GEGL_OBJECT(buffer1));
}

void
copy_ops(void)
{
  GeglColorModel *cm =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);

  GeglImage *fill_op; 
  GeglImage *print_op; 
  GeglImage *copy_op; 
  GeglImage *buffer1 = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
  GeglImage *buffer2 = GEGL_IMAGE(gegl_image_buffer_new(cm,2,2));
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /* fill buffer1 */ 
  fill_op = fill_op_new(cm, .1, .2, .3, .4);
  gegl_image_get_pixels (fill_op,buffer1,&roi);

  /* copy op, put in buffer2 */
  copy_op = GEGL_IMAGE(gegl_copy_op_new(buffer1)); 
  gegl_image_get_pixels (copy_op,buffer2,&roi);

  /* print buffer2 */
  print_op = GEGL_IMAGE(gegl_print_op_new(buffer2));
  gegl_image_get_pixels (print_op,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print_op));
  gegl_object_unref (GEGL_OBJECT(copy_op));
  gegl_object_unref (GEGL_OBJECT(fill_op));
  gegl_object_unref (GEGL_OBJECT(buffer2));
  gegl_object_unref (GEGL_OBJECT(buffer1));
}

void
color_model_attributes_tests (void)
{
  GList *list = NULL;
  GeglColorModel *cm1 =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_RGB_U8);
  GeglColorModel *cm2 =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_GRAY_U16);
  GeglColorModel *cm3 =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT);

  GeglImage *image1 = GEGL_IMAGE(gegl_image_buffer_new(cm1,2,2));
  GeglImage *image2 = GEGL_IMAGE(gegl_image_buffer_new(cm2,2,2));
  GeglImage *image3 = GEGL_IMAGE(gegl_image_buffer_new(cm3,2,2));

  GeglColorSpace space1 = gegl_color_model_color_space(cm1);
  GeglColorSpace space2 = gegl_color_model_color_space(cm2);
  GeglColorSpace space3 = gegl_color_model_color_space(cm3);

  printf("actual color space1 is %d\n", space1);
  printf("actual color space2 is %d\n", space2);
  printf("actual color space3 is %d\n", space3);

  list = g_list_append(list, image1);
  list = g_list_append(list, image2);
  list = g_list_append(list, image3);

    {
      GeglColorSpace space = gegl_utils_derived_color_space(list);
      GeglChannelDataType  type = gegl_utils_derived_channel_data_type(list);
      GeglColorAlphaSpace  caspace = gegl_utils_derived_color_alpha_space(list);

      printf("derived color space is %d\n", space);
      printf("derived data type is %d\n", type);
      printf("derived color alpha space is %d\n", caspace);
    }

  g_list_free(list);
  gegl_object_unref (GEGL_OBJECT(image1));
  gegl_object_unref (GEGL_OBJECT(image2));
  gegl_object_unref (GEGL_OBJECT(image3));

}

void
dual_src_point_ops(void)
{
  GeglColorModel *rgb_float = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);

  gint w = 2;
  gint h = 2;

  test_dual_src_point_op (ADD,rgb_float,w,h);
  test_dual_src_point_op (DARK,rgb_float,w,h);
  test_dual_src_point_op (DIFF,rgb_float,w,h);
  test_dual_src_point_op (LIGHT,rgb_float,w,h);
  test_dual_src_point_op (MIN,rgb_float,w,h);
  test_dual_src_point_op (MAX,rgb_float,w,h);
  test_dual_src_point_op (MULT,rgb_float,w,h);
  test_dual_src_point_op (SUBTRACT,rgb_float,w,h);
  test_dual_src_point_op (SCREEN,rgb_float,w,h);
}

void
single_src_point_ops(void)
{
  GeglColorModel *rgba_float = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT);
  gint w = 2;
  gint h = 2;

  test_single_src_point_op (PREMULT,rgba_float,w,h);
  test_single_src_point_op (TEST,rgba_float,w,h);
  test_single_src_point_op (UNPREMULT,rgba_float,w,h);
}

void
test_composite_ops(void)
{
  GeglColorModel *rgba_float = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT);
  gint w = 2;
  gint h = 2;

  test_composite_op (COMP_ATOP,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_IN,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_OUT,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_OVER,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_REPLACE,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_XOR,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_ATOP,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_IN,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_OUT,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_OVER,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_REPLACE,rgba_float,rgba_float,w,h);
  test_composite_op (COMP_PREMULT_XOR,rgba_float,rgba_float,w,h);
}

int
main (int argc, char *argv[])
{  
  /*Makes efence work*/
  int *p = malloc(sizeof(int) * 2); 
  p = NULL;

  gtk_init (&argc, &argv);
  gegl_init(&argc, &argv);

  /* simple_tree(); */

  /* some tests */
#if 1 
  simple_fill();
  fill_buffer_and_print();
  linear_chain_test();
  simple_tree();
  color_convert_ops();
  copy_ops();
  color_model_attributes_tests();
  dual_src_point_ops();
  single_src_point_ops();
  test_composite_ops();
#endif

  return 0;
}
