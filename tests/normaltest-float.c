#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

#define R0 .1 
#define G0 .2
#define B0 .3 
#define A0 .4 

#define R1 .5 
#define G1 .6
#define B1 .7 
#define A1 .8  

static void
test_normal_g_object_new(Test *test)
{
  {
    GeglOp * normal = g_object_new (GEGL_TYPE_NORMAL, NULL);  

    ct_test(test, GEGL_IS_NORMAL(normal));
    ct_test(test, g_type_parent(GEGL_TYPE_NORMAL) == GEGL_TYPE_BLEND);
    ct_test(test, !strcmp("GeglNormal", g_type_name(GEGL_TYPE_NORMAL)));

    g_object_unref(normal);
  }
}

/*
     foreground (f,fa) 
     background (b,ba)
     result (c,ca) 

     all premultiplied

     Full blending equation: 
     c = (1-ba)*f + (1-fa)*b + fa*ba*blend(f/fa, b/ba)
     ca = fa + ba - fa*ba

     For normal mode, blend(x,y) = x 

     so normal mode gives:
     c = (1-fa)*b + f
     ca = fa + ba - fa*ba
*/

static void
test_normal_apply_rgb_rgb(Test *test)
{
  GeglOp *background = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgb-float", R0, G0, B0, 
                                    NULL); 

  GeglOp *foreground = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgb-float", R1, G1, B1, 
                                    NULL); 

  {
    /* 
       (b) = (R0,G0,B0) = (.1,.2,.3)
       (f) = (R1,G1,B1) = (.5,.6,.7)

       and normal mode gives:
       c = f = (.5,.6,.7)
       ca = 1.0 (unused)
     */

    GeglOp * normal = g_object_new (GEGL_TYPE_NORMAL, 
                                    "background", background,
                                    "foreground", foreground,
                                    NULL);  
                                    

    gegl_op_apply(normal); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(normal), 
                            R1,G1,B1));
    g_object_unref(normal);
  }

  g_object_unref(foreground);
  g_object_unref(background);
}

static void
test_normal_apply_rgba_rgb(Test *test)
{
  GeglOp *background = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgb-float", R0, G0, B0, 
                                    NULL); 

  GeglOp *foreground = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgba-float", R1, G1, B1, A1, 
                                    NULL); 
  {
    /* 
       (b)  = (R0,G0,B0) = (.1,.2,.3)
       (f, fa) = (R1,G1,B1,A1) = (.5,.6,.7,.8)

       c = (1-fa)*b + f;
       ca = 1.0 (unused);

       (c) = (.2*.1 + .5, .2*.2 + .6, .2*.3 + .7)
           = (.52,.64,.76)

    */

    GeglOp * normal = g_object_new (GEGL_TYPE_NORMAL, 
                                    "background", background,
                                    "foreground", foreground,
                                    NULL);  

    gegl_op_apply(normal); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(normal), 
                                   .52,.64,.76));
    g_object_unref(normal);
  }

  g_object_unref(foreground);
  g_object_unref(background);
}

static void
test_normal_apply_rgb_rgba(Test *test)
{
  GeglOp *background = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgba-float", R0, G0, B0, A0, 
                                    NULL); 

  GeglOp *foreground = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgb-float", R1, G1, B1, 
                                    NULL); 
  {
    /* 
       (b, ba) = (R0,G0,B0,A0) = (.1,.2,.3,.4)
       (f) = (R1,G1,B1) = (.5,.6,.7)

       c = f;
       ca = 1.0;

       (c, ca) = (.5, .6, .7, 1.0)

    */

    GeglOp * normal = g_object_new (GEGL_TYPE_NORMAL, 
                                    "background", background,
                                    "foreground", foreground,
                                    NULL);  

    gegl_op_apply(normal); 

    ct_test(test, testutils_check_pixel_rgba_float(GEGL_IMAGE_OP(normal), 
                                   R1,G1,B1,1.0));
    g_object_unref(normal);
  }

  g_object_unref(foreground);
  g_object_unref(background);
}

static void
test_normal_apply_rgba_rgba(Test *test)
{
  GeglOp *background = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgba-float", R0, G0, B0, A0, 
                                    NULL); 

  GeglOp *foreground = g_object_new(GEGL_TYPE_COLOR, 
                                    "width", IMAGE_OP_WIDTH, 
                                    "height", IMAGE_OP_HEIGHT, 
                                    "pixel-rgba-float", R1, G1, B1, A1, 
                                    NULL); 
  {
    /* 
       (b, ba) = (R0,G0,B0,A0) = (.1,.2,.3,.4)
       (f, fa) = (R1,G1,B1,A1) = (.5,.6,.7,.8)

       c = (1-fa)*b + f;
       ca = fa + ba - fa*ba;

       (c, ca) = (.2*.1 + .5, .2*.2 + .6, .2*.3 + .7, .8 + .4 - .8*.4)
               = (.52,.64,.76,.88)

    */

    GeglOp * normal = g_object_new (GEGL_TYPE_NORMAL, 
                                    "background", background,
                                    "foreground", foreground,
                                    NULL);  

    gegl_op_apply(normal); 

    ct_test(test, testutils_check_pixel_rgba_float(GEGL_IMAGE_OP(normal), 
                                   .52,.64,.76,.88));
    g_object_unref(normal);
  }

  g_object_unref(foreground);
  g_object_unref(background);
}

static void
normal_test_setup(Test *test)
{
}

static void
normal_test_teardown(Test *test)
{
}

Test *
create_normal_test_float()
{
  Test* t = ct_create("GeglNormalTestFloat");

  g_assert(ct_addSetUp(t, normal_test_setup));
  g_assert(ct_addTearDown(t, normal_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_normal_g_object_new));
  g_assert(ct_addTestFun(t, test_normal_apply_rgb_rgb));
  g_assert(ct_addTestFun(t, test_normal_apply_rgba_rgb));
  g_assert(ct_addTestFun(t, test_normal_apply_rgb_rgba));
  g_assert(ct_addTestFun(t, test_normal_apply_rgba_rgba));
#endif

  return t; 
}
