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
test_needrect_op(Test *t)
{
  GeglRect need_rect;
  GeglRect source0_need_rect;
  GeglRect source1_need_rect;

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              NULL);  
 
  gegl_rect_set(&need_rect, 0,0,5,5);
  gegl_rect_set(&source0_need_rect, 0,0,0,0);

  gegl_filter_compute_need_rect(GEGL_FILTER(op), &source0_need_rect, &need_rect, 0); 
  ct_test(t, gegl_rect_equal_coords(&source0_need_rect, 0,0,5,5));  

  gegl_rect_set(&source1_need_rect, 0,0,0,0);
  gegl_filter_compute_need_rect(GEGL_FILTER(op), &source1_need_rect, &need_rect, 1); 
  ct_test(t, gegl_rect_equal_coords(&source1_need_rect, 0,0,5,5));  

  g_object_unref(op);
}

static void
test_needrect_op_source_needrect_set(Test *t)
{
  GeglRect need_rect;
  GeglRect source0_need_rect;
  GeglRect source1_need_rect;

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              NULL);  
 
  gegl_rect_set(&need_rect, 1,1,5,5);
  gegl_rect_set(&source0_need_rect, 0,0,3,3);

  gegl_filter_compute_need_rect(GEGL_FILTER(op), &source0_need_rect, &need_rect, 0); 
  ct_test(t, gegl_rect_equal_coords(&source0_need_rect, 0,0,6,6));  

  gegl_rect_set(&source1_need_rect, 2,2,6,6);
  gegl_filter_compute_need_rect(GEGL_FILTER(op), &source1_need_rect, &need_rect, 1); 
  ct_test(t, gegl_rect_equal_coords(&source1_need_rect, 1,1,7,7));  

  g_object_unref(op);
}

static void
needrect_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

}

static void
needrect_test_teardown(Test *test)
{
  g_object_unref(dest);
}

Test *
create_needrect_test()
{
  Test* t = ct_create("GeglNeedRectTest");

  g_assert(ct_addSetUp(t, needrect_test_setup));
  g_assert(ct_addTearDown(t, needrect_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_needrect_op));
  g_assert(ct_addTestFun(t, test_needrect_op_source_needrect_set));
#endif

  return t; 
}
