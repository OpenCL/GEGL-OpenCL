#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define MULTIPLIER .5

GeglSimpleImageMgr *simple_image_man; 
GeglSampledImage * source_sampled_image;
GeglSampledImage * dest_sampled_image;

static void
test_const_mult_impl_g_object_new(Test *test)
{
  {
    GeglConstMultImpl * const_mult_impl = g_object_new (GEGL_TYPE_CONST_MULT_IMPL, NULL);  

    ct_test(test, const_mult_impl != NULL);
    ct_test(test, GEGL_IS_CONST_MULT_IMPL(const_mult_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_CONST_MULT_IMPL) == GEGL_TYPE_POINT_OP_IMPL);
    ct_test(test, !strcmp("GeglConstMultImpl", g_type_name(GEGL_TYPE_CONST_MULT_IMPL)));

    g_object_unref(const_mult_impl);
  }
}

static void
test_const_mult_impl_set_multiplier(Test *test)
{
  {
    GeglConstMultImpl * const_mult_impl = g_object_new (GEGL_TYPE_CONST_MULT_IMPL, NULL);  
    gegl_const_mult_impl_set_multiplier(const_mult_impl, MULTIPLIER); 
    ct_test(test, MULTIPLIER == gegl_const_mult_impl_get_multiplier(const_mult_impl));
    g_object_unref(const_mult_impl);
  }
}

static void
const_mult_impl_test_setup(Test *test)
{
}

static void
const_mult_impl_test_teardown(Test *test)
{
}

Test *
create_const_mult_impl_test()
{
  Test* t = ct_create("GeglConstMultImplTest");

  g_assert(ct_addSetUp(t, const_mult_impl_test_setup));
  g_assert(ct_addTearDown(t, const_mult_impl_test_teardown));

  g_assert(ct_addTestFun(t, test_const_mult_impl_g_object_new));
  g_assert(ct_addTestFun(t, test_const_mult_impl_set_multiplier));

  return t; 
}
