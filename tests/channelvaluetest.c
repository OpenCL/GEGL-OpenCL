#include <glib-object.h>
#include "gegl-mock-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

static void
test_channel_param_spec(Test *test)
{
  GParamSpec *pspec =  gegl_param_spec_channel("blah", 
                                               "Blah",
                                               "This is channel data",
                                                G_PARAM_READWRITE);
  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  ct_test(test, GEGL_TYPE_CHANNEL == G_PARAM_SPEC_VALUE_TYPE(pspec));

  g_param_spec_unref(pspec);
}

static void
test_channel_param_spec_validate(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_channel("blah", 
                                               "Blah",
                                               "This is channel data",
                                                G_PARAM_READWRITE);

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_CHANNEL_FLOAT);
  g_value_set_channel_float(value, .33);

  ct_test(test, GEGL_TYPE_CHANNEL == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, !g_param_value_validate(pspec, value));

  g_param_spec_unref(pspec);

  g_value_unset(value);
  g_free(value);
}

static void
channel_value_test_setup(Test *test)
{
}

static void
channel_value_test_teardown(Test *test)
{
}

Test *
create_channel_value_test()
{
  Test* t = ct_create("GeglChannelValueTest");

  g_assert(ct_addSetUp(t, channel_value_test_setup));
  g_assert(ct_addTearDown(t, channel_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_channel_param_spec));
  g_assert(ct_addTestFun(t, test_channel_param_spec_validate));
#endif

  return t; 
}
