#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_data_space_g_object_new(Test *test)
{
  {
    GeglDataSpace * data_space = g_object_new (GEGL_TYPE_DATA_SPACE_FLOAT, NULL);  

    ct_test(test, data_space != NULL);
    ct_test(test, GEGL_IS_DATA_SPACE(data_space));
    ct_test(test, GEGL_IS_DATA_SPACE_FLOAT(data_space));
    ct_test(test, g_type_parent(GEGL_TYPE_DATA_SPACE) == GEGL_TYPE_OBJECT);
    ct_test(test, g_type_parent(GEGL_TYPE_DATA_SPACE_FLOAT) == GEGL_TYPE_DATA_SPACE);
    ct_test(test, !strcmp("GeglDataSpaceFloat", g_type_name(GEGL_TYPE_DATA_SPACE_FLOAT)));
    ct_test(test, !strcmp("GeglDataSpace", g_type_name(GEGL_TYPE_DATA_SPACE)));

    g_object_unref(data_space);
  }
}

static void
test_data_space_float(Test *test)
{
  {
    GeglDataSpace * data_space = g_object_new (GEGL_TYPE_DATA_SPACE_FLOAT, NULL);  

    ct_test(test, GEGL_DATA_SPACE_FLOAT == gegl_data_space_data_space_type(data_space)); 
    ct_test(test, 32 == gegl_data_space_bits(data_space)); 
    ct_test(test, TRUE == gegl_data_space_is_channel_data(data_space)); 
    ct_test(test, !strcmp("float", gegl_data_space_name(data_space)));

    g_object_unref(data_space);
  }
}

static void
test_data_space_u8(Test *test)
{
  {
    GeglDataSpace * data_space = g_object_new (GEGL_TYPE_DATA_SPACE_U8, NULL);  

    ct_test(test, GEGL_DATA_SPACE_U8 == gegl_data_space_data_space_type(data_space)); 
    ct_test(test, 8 == gegl_data_space_bits(data_space)); 
    ct_test(test, TRUE == gegl_data_space_is_channel_data(data_space)); 
    ct_test(test, !strcmp("u8", gegl_data_space_name(data_space)));

    g_object_unref(data_space);
  }
}

static void
test_data_space_u8_convert_to_float(Test *test)
{
  {
    GeglDataSpace * data_space = g_object_new (GEGL_TYPE_DATA_SPACE_U8, NULL);  
    gfloat dest;
    guint8 src = 1;

    gegl_data_space_convert_to_float(data_space, &dest, (gpointer)&src, 1); 
    ct_test(test, GEGL_FLOAT_EQUAL(1/255.0, dest)); 

    src = 128;
    gegl_data_space_convert_to_float(data_space, &dest, (gpointer)&src, 1); 
    ct_test(test, GEGL_FLOAT_EQUAL(128/255.0, dest)); 

    g_object_unref(data_space);
  }
}

static void
test_data_space_u8_convert_from_float(Test *test)
{
  {
    GeglDataSpace * data_space = g_object_new (GEGL_TYPE_DATA_SPACE_U8, NULL);  
    gfloat src;
    guint8 dest;

    src = .5;
    gegl_data_space_convert_from_float(data_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 128 == dest); 

    src = 1.5;
    gegl_data_space_convert_from_float(data_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 255 == dest); 

    src = 1.0;
    gegl_data_space_convert_from_float(data_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 255 == dest); 

    src = -.5;
    gegl_data_space_convert_from_float(data_space, (gpointer)&dest, &src, 1); 
    ct_test(test, 0 == dest); 

    g_object_unref(data_space);
  }
}

static void
data_space_test_setup(Test *test)
{
}

static void
data_space_test_teardown(Test *test)
{
}

Test *
create_data_space_test()
{
  Test* t = ct_create("GeglDataSpaceTest");

#if 1
  g_assert(ct_addSetUp(t, data_space_test_setup));
  g_assert(ct_addTearDown(t, data_space_test_teardown));
  g_assert(ct_addTestFun(t, test_data_space_g_object_new));
  g_assert(ct_addTestFun(t, test_data_space_float));
  g_assert(ct_addTestFun(t, test_data_space_u8));
  g_assert(ct_addTestFun(t, test_data_space_u8_convert_to_float));
  g_assert(ct_addTestFun(t, test_data_space_u8_convert_from_float));
#endif

  return t; 
}
