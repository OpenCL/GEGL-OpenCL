#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_sampled_image_impl_g_object_new(Test *test)
{
  {
    GeglSampledImageImpl * sampled_image_impl = g_object_new (GEGL_TYPE_SAMPLED_IMAGE_IMPL, NULL);  

    ct_test(test, sampled_image_impl != NULL);
    ct_test(test, GEGL_IS_SAMPLED_IMAGE_IMPL(sampled_image_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_SAMPLED_IMAGE_IMPL) == GEGL_TYPE_IMAGE_IMPL);
    ct_test(test, !strcmp("GeglSampledImageImpl", g_type_name(GEGL_TYPE_SAMPLED_IMAGE_IMPL)));

    g_object_unref(sampled_image_impl);
  }
}

static void
test_sampled_image_impl_get_num_inputs(Test *test)
{
  {
    GeglSampledImageImpl * sampled_image_impl = g_object_new (GEGL_TYPE_SAMPLED_IMAGE_IMPL, NULL);  

    ct_test(test, sampled_image_impl != NULL);
    ct_test(test, 0 == gegl_op_impl_get_num_inputs(GEGL_OP_IMPL(sampled_image_impl)));

    g_object_unref(sampled_image_impl);
  }
}

static void
sampled_image_impl_test_setup(Test *test)
{
}

static void
sampled_image_impl_test_teardown(Test *test)
{
}

Test *
create_sampled_image_impl_test()
{
  Test* t = ct_create("GeglSampledImageImplTest");

  g_assert(ct_addSetUp(t, sampled_image_impl_test_setup));
  g_assert(ct_addTearDown(t, sampled_image_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_sampled_image_impl_g_object_new));
  g_assert(ct_addTestFun(t, test_sampled_image_impl_get_num_inputs));

  return t; 
}
