#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define R0 .1 
#define G0 .2
#define B0 .3 

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

static GeglOp * source;

static void
test_channels_g_object_new(Test *test)
{
  {
    GeglChannels * channels = g_object_new (GEGL_TYPE_CHANNELS, NULL);  

    ct_test(test, channels != NULL);
    ct_test(test, GEGL_IS_CHANNELS(channels));
    ct_test(test, g_type_parent(GEGL_TYPE_CHANNELS) == GEGL_TYPE_MULTI_IMAGE_OP);
    ct_test(test, !strcmp("GeglChannels", g_type_name(GEGL_TYPE_CHANNELS)));

    g_object_unref(channels);
  }
}

static void
test_channels_apply(Test *test)
{
  GeglOp *channels = g_object_new(GEGL_TYPE_CHANNELS,
                                  "source", source,
                                  NULL);

  gegl_op_apply(channels); 
  g_object_unref(channels);
}

static void
channels_test_setup(Test *test)
{
  GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                  "rgb-float", R0, G0, B0, 
                                  NULL);
  source = g_object_new(GEGL_TYPE_FILL, 
                        "width", IMAGE_OP_WIDTH, 
                        "height", IMAGE_OP_HEIGHT, 
                        "fill-color", color, 
                        NULL); 
  g_object_unref(color);
}

static void
channels_test_teardown(Test *test)
{
  g_object_unref(source);
}


Test *
create_channels_test_float()
{
  Test* t = ct_create("GeglChannelsTestFloat");

  g_assert(ct_addSetUp(t, channels_test_setup));
  g_assert(ct_addTearDown(t, channels_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_channels_g_object_new));
  g_assert(ct_addTestFun(t, test_channels_apply));
#endif

  return t; 
}
