#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 5 
#define SAMPLED_IMAGE_HEIGHT 5 
GeglSampledImage *dest;

static void
test_haverect_fill(Test *t)
{
  GeglRect have_rect;
  GeglRect default_rect = {0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT};
  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 
  gegl_filter_compute_have_rect(GEGL_FILTER(fill), &have_rect, NULL); 
  ct_test(t, gegl_rect_equal(&have_rect, &default_rect));  
  g_object_unref(fill);
}

static void
test_haverect_sampled_image(Test *t)
{
  GeglRect have_rect;
  gegl_filter_compute_have_rect(GEGL_FILTER(dest), &have_rect, NULL); 
  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0,
                                    SAMPLED_IMAGE_WIDTH,
                                    SAMPLED_IMAGE_HEIGHT));  
}

static void
test_haverect_op(Test *t)
{
  GeglRect have_rect;
  GeglRect have_rect1 = {0,0,2,2};
  GeglRect have_rect2 = {1,1,2,2};

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              NULL);  

  GList * have_rects = NULL;
  have_rects = g_list_append(have_rects, &have_rect1); 
  have_rects = g_list_append(have_rects, &have_rect2); 

  gegl_filter_compute_have_rect(GEGL_FILTER(op), &have_rect, have_rects); 
  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0,3,3));  

  g_object_unref(op);
  g_list_free(have_rects);
}

static void
haverect_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

  g_object_unref(rgb_float);
}

static void
haverect_test_teardown(Test *test)
{
  g_object_unref(dest);
}

Test *
create_haverect_test()
{
  Test* t = ct_create("GeglHaveRectTest");

  g_assert(ct_addSetUp(t, haverect_test_setup));
  g_assert(ct_addTearDown(t, haverect_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_haverect_fill));
  g_assert(ct_addTestFun(t, test_haverect_sampled_image));
  g_assert(ct_addTestFun(t, test_haverect_op));
#endif

  return t; 
}
