#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define IMAGE_WIDTH 5 
#define IMAGE_HEIGHT 5 

#define R0 .1 
#define G0 .2
#define B0 .3 

#define R1 .4 
#define G1 .5
#define B1 .6  

static void
test_haverect_color_default_size(Test *t)
{
  GeglRect have_rect;
  GeglRect default_rect = {0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT};

  GeglFilter * source = g_object_new(GEGL_TYPE_COLOR, 
                                     "pixel-rgb-float", R0, G0, B0, 
                                     NULL); 

  gegl_filter_compute_have_rect(source, &have_rect, NULL); 

  ct_test(t, gegl_rect_equal(&have_rect, &default_rect));  

  g_object_unref(source);
}

static void
test_haverect_color(Test *t)
{
  GeglRect have_rect;
  GeglFilter * source = g_object_new(GEGL_TYPE_COLOR, 
                                     "width", IMAGE_WIDTH, 
                                     "height", IMAGE_HEIGHT, 
                                     "pixel-rgb-float", R0, G0, B0, 
                                     NULL); 

  gegl_filter_compute_have_rect(source, &have_rect, NULL); 

  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0, IMAGE_WIDTH, IMAGE_HEIGHT));  

  g_object_unref(source);
}

static void
test_haverect_op(Test *t)
{
  GeglRect have_rect;
  GeglRect have_rect1 = {0,0,2,2};
  GeglRect have_rect2 = {1,1,2,2};

  GeglFilter * source = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                                      "num_inputs", 2,
                                      NULL);  

  GList * have_rects = NULL;
  have_rects = g_list_append(have_rects, &have_rect1); 
  have_rects = g_list_append(have_rects, &have_rect2); 

  gegl_filter_compute_have_rect(source, &have_rect, have_rects); 
  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0,3,3));  

  g_object_unref(source);
  g_list_free(have_rects);
}

static void
haverect_test_setup(Test *test)
{
}

static void
haverect_test_teardown(Test *test)
{
}

Test *
create_haverect_test()
{
  Test* t = ct_create("GeglHaveRectTest");

  g_assert(ct_addSetUp(t, haverect_test_setup));
  g_assert(ct_addTearDown(t, haverect_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_haverect_color_default_size));
  g_assert(ct_addTestFun(t, test_haverect_color));
  g_assert(ct_addTestFun(t, test_haverect_op));
#endif

  return t; 
}
