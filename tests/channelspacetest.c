#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_channel_space_g_object_new(Test *test)
{
  {
    GeglChannelSpace * channel_space = g_object_new (GEGL_TYPE_CHANNEL_SPACE_FLOAT, NULL);  

    ct_test(test, channel_space != NULL);
    ct_test(test, GEGL_IS_CHANNEL_SPACE(channel_space));
    ct_test(test, GEGL_IS_CHANNEL_SPACE_FLOAT(channel_space));
    ct_test(test, g_type_parent(GEGL_TYPE_CHANNEL_SPACE) == GEGL_TYPE_OBJECT);
    ct_test(test, g_type_parent(GEGL_TYPE_CHANNEL_SPACE_FLOAT) == GEGL_TYPE_CHANNEL_SPACE);
    ct_test(test, !strcmp("GeglChannelSpaceFloat", g_type_name(GEGL_TYPE_CHANNEL_SPACE_FLOAT)));
    ct_test(test, !strcmp("GeglChannelSpace", g_type_name(GEGL_TYPE_CHANNEL_SPACE)));

    g_object_unref(channel_space);
  }
}

static void
test_channel_space_float(Test *test)
{
  {
    GeglChannelSpace * channel_space = g_object_new (GEGL_TYPE_CHANNEL_SPACE_FLOAT, NULL);  

    ct_test(test, GEGL_CHANNEL_SPACE_FLOAT == gegl_channel_space_channel_space_type(channel_space)); 
    ct_test(test, 32 == gegl_channel_space_bits(channel_space)); 
    ct_test(test, TRUE == gegl_channel_space_is_channel_data(channel_space)); 
    ct_test(test, !strcmp("float", gegl_channel_space_name(channel_space)));

    g_object_unref(channel_space);
  }
}

static void
test_channel_space_uint8(Test *test)
{
  {
    GeglChannelSpace * channel_space = g_object_new (GEGL_TYPE_CHANNEL_SPACE_UINT8, NULL);  

    ct_test(test, GEGL_CHANNEL_SPACE_UINT8 == gegl_channel_space_channel_space_type(channel_space)); 
    ct_test(test, 8 == gegl_channel_space_bits(channel_space)); 
    ct_test(test, TRUE == gegl_channel_space_is_channel_data(channel_space)); 
    ct_test(test, !strcmp("uint8", gegl_channel_space_name(channel_space)));

    g_object_unref(channel_space);
  }
}

static void
test_channel_space_uint8_convert_to_float(Test *test)
{
  {
    GeglChannelSpace * channel_space = g_object_new (GEGL_TYPE_CHANNEL_SPACE_UINT8, NULL);  
    gfloat dest;
    guint8 src = 1;

    gegl_channel_space_convert_to_float(channel_space, &dest, (gpointer)&src, 1); 
    ct_test(test, GEGL_FLOAT_EQUAL(1/255.0, dest)); 

    src = 128;
    gegl_channel_space_convert_to_float(channel_space, &dest, (gpointer)&src, 1); 
    ct_test(test, GEGL_FLOAT_EQUAL(128/255.0, dest)); 

    g_object_unref(channel_space);
  }
}

static void
test_channel_space_uint8_convert_from_float(Test *test)
{
  {
    GeglChannelSpace * channel_space = g_object_new (GEGL_TYPE_CHANNEL_SPACE_UINT8, NULL);  
    gfloat src;
    guint8 dest;

    src = .5;
    gegl_channel_space_convert_from_float(channel_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 128 == dest); 

    src = 1.5;
    gegl_channel_space_convert_from_float(channel_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 255 == dest); 

    src = 1.0;
    gegl_channel_space_convert_from_float(channel_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 255 == dest); 

    src = -.5;
    gegl_channel_space_convert_from_float(channel_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 0 == dest); 

    g_object_unref(channel_space);
  }
}

static void
channel_space_test_setup(Test *test)
{
}

static void
channel_space_test_teardown(Test *test)
{
}

Test *
create_channel_space_test()
{
  Test* t = ct_create("GeglChannelSpaceTest");

#if 1
  g_assert(ct_addSetUp(t, channel_space_test_setup));
  g_assert(ct_addTearDown(t, channel_space_test_teardown));
  g_assert(ct_addTestFun(t, test_channel_space_g_object_new));
  g_assert(ct_addTestFun(t, test_channel_space_float));
  g_assert(ct_addTestFun(t, test_channel_space_uint8));
  g_assert(ct_addTestFun(t, test_channel_space_uint8_convert_to_float));
  g_assert(ct_addTestFun(t, test_channel_space_uint8_convert_from_float));
#endif

  return t; 
}
