#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>
#include <stdlib.h>
#include <gegl.h>

#define TEST_GEGL_LOG_DEBUG(args...) g_log("testgegl", G_LOG_LEVEL_DEBUG, ##args)
#define TEST_GEGL_LOG_INFO(args...) g_log("testgegl", G_LOG_LEVEL_INFO, ##args)

GeglOp * 
fill_op_new(GeglColorModel *cm,
            gfloat a, 
            gfloat b,
            gfloat c,
            gfloat d)
{
  gint num_chan = gegl_color_model_num_channels(cm);
  GeglColor *color = gegl_color_new (cm);
  GeglChannelValue * chans = gegl_color_get_channel_values(color);
  GeglOp *op;

  /* Fill image with (a,b,c,d) */
  chans[0].f = a;
  if (num_chan > 1)
    chans[1].f = b;
  if (num_chan > 2)
    chans[2].f = c;
  if (num_chan > 3)
    chans[3].f = d;

  op = GEGL_OP(gegl_fill_op_new (color)); 
  gegl_object_unref (GEGL_OBJECT(color));

  return op;
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
  GeglOp *fill = GEGL_OP(gegl_fill_op_new (color)); 

  /* Fill image with (a,b,c,d) */
  chans[0].f = a;
  if (num_chan > 1)
    chans[1].f = b;
  if (num_chan > 2)
    chans[2].f = c;
  if (num_chan > 3)
    chans[3].f = d;

  gegl_op_apply (fill,image_buffer,NULL);

  gegl_object_unref (GEGL_OBJECT(color));
  gegl_object_unref (GEGL_OBJECT(fill));
} 

void
simple_fill(void)
{
  GeglOp *fill; 
  GeglOp *print; 
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /* fill and print it */
  fill = GEGL_OP(fill_op_new(cm, .1, .2, .3, .4));
  print = GEGL_OP(gegl_print_op_new(fill));
  gegl_op_apply(print, NULL, &roi); 

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(fill));
}

void
linear_chain_test(void)
{
  GeglOp *fill;
  GeglOp *test1, *test2;
  GeglOp *print;
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);

  GeglImageBuffer *buffer = gegl_image_buffer_new (cm,2,2);
  GeglRect roi;
  gegl_rect_set (&roi,0,0,2,2);

  /*
    test2 -> buffer 
      |   
    test1
      |
    fill  

  */ 

  fill = fill_op_new(cm, .1, .2, .3, .4);

  test1 = GEGL_OP(gegl_test_op_new(fill)); 
  test2 = GEGL_OP(gegl_test_op_new(test1)); 

  gegl_op_apply (test2,buffer,&roi);

  /* print it */
  print = GEGL_OP (gegl_print_op_new (GEGL_OP (buffer))); 
  gegl_op_apply (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT (print));
  gegl_object_unref (GEGL_OBJECT (fill));
  gegl_object_unref (GEGL_OBJECT (test1));
  gegl_object_unref (GEGL_OBJECT (test2));
  gegl_object_unref (GEGL_OBJECT (buffer));
}

void
simple_tree(void)
{
  GeglOp *fill1, *fill2;
  GeglOp *add;
  GeglOp *print;
  GeglOp *test;
  GeglColorModel *cm = gegl_color_model_instance(
                            GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglImageBuffer *buffer = gegl_image_buffer_new(cm,2,2);
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  /*
        add -> buffer
        /  \ 
    test   fill2 
      |
    fill1  

          (.55,.7,.85)
          /         \
    (.05,.1,.15)   (.5,.6,.7)
         | 
    (.1,.2,.3)

  */ 

  /* Evaluate the graph, put in buffer */
  fill1 = fill_op_new(cm, .1, .2, .3, .4);
  fill2 = fill_op_new(cm, .5, .6, .7, .8);
  test = GEGL_OP(gegl_test_op_new(fill1)); 
  add = GEGL_OP(gegl_add_op_new(fill2, test)); 
  gegl_op_apply (add, buffer, &roi);

  /* print buffer */
  print = GEGL_OP(gegl_print_op_new(GEGL_OP(buffer))); 
  gegl_op_apply (print,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(add));
  gegl_object_unref (GEGL_OBJECT(test));
  gegl_object_unref (GEGL_OBJECT(fill1));
  gegl_object_unref (GEGL_OBJECT(fill2));
  gegl_object_unref (GEGL_OBJECT(buffer));
}

void
copy_ops(void)
{
  GeglColorModel *cm = 
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);

  GeglOp *fill; 
  GeglOp *print; 
  GeglOp *copy; 

  GeglImageBuffer *buffer1 = gegl_image_buffer_new(cm,2,2);
  GeglImageBuffer *buffer2 = gegl_image_buffer_new(cm,2,2);
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  TEST_GEGL_LOG_DEBUG("Filling to buffer1");

  /* fill buffer1 */ 
  fill = fill_op_new (cm, .1, .2, .3, .4);
  gegl_op_apply (fill, buffer1, &roi);

  TEST_GEGL_LOG_DEBUG("Copying buffer1 to buffer2");

  /* copy, put in buffer2 */
  copy = GEGL_OP (gegl_copy_op_new (GEGL_OP(buffer1))); 
  gegl_op_apply (copy, buffer2, &roi);

  TEST_GEGL_LOG_DEBUG("Printing buffer2");

  /* print buffer2 */
  print = GEGL_OP (gegl_print_op_new (GEGL_OP(buffer2)));
  gegl_op_apply (print, NULL, &roi);

  gegl_object_unref (GEGL_OBJECT(print));
  gegl_object_unref (GEGL_OBJECT(copy));
  gegl_object_unref (GEGL_OBJECT(fill));
  gegl_object_unref (GEGL_OBJECT(buffer2));
  gegl_object_unref (GEGL_OBJECT(buffer1));
}

void
color_convert_ops (void)
{
  GeglColorModel *rgb_float =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_RGB_FLOAT);
  GeglColorModel *gray_float =
    gegl_color_model_instance(GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT);

  GeglOp *fill_op; 
  GeglOp *print_op; 
  GeglOp *convert_op; 
  GeglImageBuffer *buffer1 = gegl_image_buffer_new(rgb_float,2,2);
  GeglImageBuffer *buffer2 = gegl_image_buffer_new(gray_float,2,2);
  GeglRect roi;
  gegl_rect_set(&roi,0,0,2,2);

  TEST_GEGL_LOG_DEBUG("Fill buffer1");

  /* fill buffer1 */ 
  fill_op = fill_op_new (rgb_float, .1, .2, .3, .4);
  gegl_op_apply (fill_op,buffer1,&roi);

  TEST_GEGL_LOG_DEBUG("Do color convert, put in buffer2");

  /* color convert, put in buffer2 */
  convert_op = GEGL_OP(gegl_color_convert_op_new (GEGL_OP(buffer1), gray_float));
  gegl_op_apply (convert_op,buffer2,&roi);

  TEST_GEGL_LOG_DEBUG("Print buffer2");

  /* print buffer2 */
  print_op = GEGL_OP(gegl_print_op_new(GEGL_OP(buffer2)));
  gegl_op_apply (print_op,NULL,&roi);

  gegl_object_unref (GEGL_OBJECT(print_op));
  gegl_object_unref (GEGL_OBJECT(convert_op));
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

  TEST_GEGL_LOG_DEBUG("Actual color space1 is %d", space1);
  TEST_GEGL_LOG_DEBUG("Actual color space2 is %d", space2);
  TEST_GEGL_LOG_DEBUG("Actual color space3 is %d", space3);

  list = g_list_append(list, image1);
  list = g_list_append(list, image2);
  list = g_list_append(list, image3);

    {
      GeglColorSpace space = gegl_utils_derived_color_space(list);
      GeglChannelDataType  type = gegl_utils_derived_channel_data_type(list);
      GeglColorAlphaSpace  caspace = gegl_utils_derived_color_alpha_space(list);

      TEST_GEGL_LOG_DEBUG("derived color space is %d", space);
      TEST_GEGL_LOG_DEBUG("derived data type is %d", type);
      TEST_GEGL_LOG_DEBUG("derived color alpha space is %d", caspace);
    }

  g_list_free(list);
  gegl_object_unref (GEGL_OBJECT(image1));
  gegl_object_unref (GEGL_OBJECT(image2));
  gegl_object_unref (GEGL_OBJECT(image3));

}

typedef enum
{
  PREMULT,
  TEST,
  UNPREMULT
}SingleSrcPointOpType;

GeglOp *
single_src_point_op_factory (SingleSrcPointOpType type,
                             GeglOp* s)
{
  switch (type)
    {
    case PREMULT:
       return GEGL_OP(gegl_premult_op_new (s)); 
    case TEST:
       return GEGL_OP(gegl_test_op_new (s)); 
    case UNPREMULT:
       return GEGL_OP(gegl_unpremult_op_new (s)); 
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

GeglOp *
dual_src_point_op_factory (DualSrcPointOpType type,
                           GeglOp* s1,
                           GeglOp* s2)
{
  switch (type)
    {
    case ADD:
       return GEGL_OP(gegl_add_op_new (s1, s2)); 
    case DARK:
       return GEGL_OP(gegl_dark_op_new (s1, s2)); 
    case DIFF:
       return GEGL_OP(gegl_diff_op_new (s1, s2)); 
    case LIGHT:
       return GEGL_OP(gegl_light_op_new (s1, s2)); 
    case MIN:
       return GEGL_OP(gegl_min_op_new (s1, s2));
    case MAX:
       return GEGL_OP(gegl_max_op_new (s1, s2));
    case MULT:
       return GEGL_OP(gegl_mult_op_new (s1, s2)); 
    case SUBTRACT:
       return GEGL_OP(gegl_subtract_op_new (s1, s2)); 
    case SCREEN:
       return GEGL_OP(gegl_screen_op_new (s1, s2)); 
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

GeglOp *
composite_op_factory (CompositeOpType type,
                      gchar **type_name,
                      GeglOp* s0,
                      GeglOp* s1)
{
  switch (type)
    {
    case COMP_ATOP:
       *type_name = g_strdup("atop");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_ATOP)); 
    case COMP_IN:
       *type_name = g_strdup("in");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_IN)); 
    case COMP_OUT:
       *type_name = g_strdup("out");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_OUT)); 
    case COMP_OVER:
       *type_name = g_strdup("over");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_OVER)); 
    case COMP_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_REPLACE)); 
    case COMP_XOR:
       *type_name = g_strdup("xor");
       return GEGL_OP(gegl_composite_op_new (s0, s1, GEGL_COMPOSITE_XOR)); 
    case COMP_PREMULT_ATOP:
       *type_name = g_strdup("atop");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_ATOP)); 
    case COMP_PREMULT_IN:
       *type_name = g_strdup("in");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_IN)); 
    case COMP_PREMULT_OUT:
       *type_name = g_strdup("out");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_OUT)); 
    case COMP_PREMULT_OVER:
       *type_name = g_strdup("over");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_OVER)); 
    case COMP_PREMULT_REPLACE:
       *type_name = g_strdup("replace");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_REPLACE)); 
    case COMP_PREMULT_XOR:
       *type_name = g_strdup("xor");
       return GEGL_OP(gegl_composite_premult_op_new (s0, s1, GEGL_COMPOSITE_XOR)); 
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
  GeglImageBuffer *A = gegl_image_buffer_new(cm,width,height);
  GeglImageBuffer *B = gegl_image_buffer_new(cm,width,height);
  GeglOp *print;

  GeglOp *op = single_src_point_op_factory (type, GEGL_OP(A));
  GeglRect rect;
  gegl_rect_set (&rect, 0, 0, width, height);

  /* Set up A, clear B */
  fill_image (A, .1, .2, .3, .4);
  fill_image (B, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in B */
  gegl_op_apply (op, B, &rect);

  TEST_GEGL_LOG_DEBUG("Result of %s: ", gegl_object_type_name(GEGL_OBJECT(op)));

  print = GEGL_OP(gegl_print_op_new(GEGL_OP(B)));
  gegl_op_apply (print,NULL,&rect);

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
  gegl_object_unref (GEGL_OBJECT(print));
}

void
test_dual_src_point_op(DualSrcPointOpType type, 
                       GeglColorModel *cm,
                       gint width, 
                       gint height)
{
  GeglImageBuffer *A = gegl_image_buffer_new(cm,width,height);
  GeglImageBuffer *B = gegl_image_buffer_new(cm,width,height);
  GeglImageBuffer *C = gegl_image_buffer_new(cm,width,height);

  GeglOp *op = dual_src_point_op_factory(type, GEGL_OP(A), GEGL_OP(B));
  GeglOp *print;

  GeglRect rect;
  gegl_rect_set(&rect, 0,0,width,height);

  /* Set up A, B, C. */
  fill_image(A, .1, .2, .3, .4);
  fill_image(B, .5, .6, .7, .8);
  fill_image(C, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in C */
  gegl_op_apply (op, C, &rect);

  TEST_GEGL_LOG_DEBUG("Result of %s: ", gegl_object_type_name(GEGL_OBJECT(op)));

  print = GEGL_OP(gegl_print_op_new(GEGL_OP(C)));
  gegl_op_apply (print,NULL,&rect);

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(C));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
  gegl_object_unref (GEGL_OBJECT(print));
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
  GeglImageBuffer *A = gegl_image_buffer_new(s0_cm,width,height);
  GeglImageBuffer *B = gegl_image_buffer_new(s1_cm,width,height);
  GeglImageBuffer *C = gegl_image_buffer_new(s0_cm,width,height);

  GeglOp *op = composite_op_factory(type, &type_name, GEGL_OP(A),GEGL_OP(B));
  GeglOp *print;

  GeglRect rect;
  gegl_rect_set(&rect, 0,0,width,height);

  /* Set up C  = B op A */
  fill_image(A, .1, .2, .3, .4);
  fill_image(B, .5, .6, .7, .8);
  fill_image(C, 0.0, 0.0, 0.0, 0.0);

  /* Do the operation, put in C */
  gegl_op_apply (op, C, &rect);

  TEST_GEGL_LOG_DEBUG("Result of %s: ", gegl_object_type_name(GEGL_OBJECT(op)));

  print = GEGL_OP(gegl_print_op_new(GEGL_OP(C)));
  gegl_op_apply (print,NULL,&rect);

  gegl_object_unref (GEGL_OBJECT(op));
  gegl_object_unref (GEGL_OBJECT(C));
  gegl_object_unref (GEGL_OBJECT(B));
  gegl_object_unref (GEGL_OBJECT(A));
  gegl_object_unref (GEGL_OBJECT(print));
  g_free(type_name);
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

  simple_tree();
  /* some tests */
#if 0 
  color_convert_ops();
  copy_ops();
  color_model_attributes_tests();
  dual_src_point_ops();
  fill_buffer_and_print();
  linear_chain_test();
  simple_fill();
  single_src_point_ops();
  simple_tree();
  test_composite_ops();
#endif

  return 0;
}
