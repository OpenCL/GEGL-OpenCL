#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_constant_g_object_new(Test *test)
{
  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, NULL);  

    ct_test(test, constant != NULL);
    ct_test(test, GIL_IS_CONSTANT(constant));
    ct_test(test, g_type_parent(GIL_TYPE_CONSTANT) == GIL_TYPE_EXPRESSION);
    ct_test(test, !strcmp("GilConstant", g_type_name(GIL_TYPE_CONSTANT)));

    g_object_unref(constant);
  }

  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_INT,
                                           "int" , 10, 
                                           NULL);  
    g_object_unref(constant);
  }

  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_FLOAT,
                                           "float" , 1.5, 
                                           NULL);  
    g_object_unref(constant);
  }
}

static void
test_constant_g_object_get(Test *test)
{
  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_INT, 
                                           "int" , 2, 
                                           NULL);  
    ct_test(test, GIL_INT == gil_constant_get_const_type(constant));
    ct_test(test, 2 == gil_constant_get_int(constant));
    g_object_unref(constant);
  }

  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_FLOAT, 
                                           "float" , 1.5, 
                                           NULL);  
    ct_test(test, GIL_FLOAT == gil_constant_get_const_type(constant));
    ct_test(test, 1.5 == gil_constant_get_float(constant));
    g_object_unref(constant);
  }
}


static void
test_constant_g_object_set(Test *test)
{
  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_INT, 
                                           NULL);  
    gil_constant_set_int(constant, 10); 
    ct_test(test, GIL_INT == gil_constant_get_const_type(constant));
    ct_test(test, 10 == gil_constant_get_int(constant));
    g_object_unref(constant);
  }

  {
    GilConstant * constant = g_object_new (GIL_TYPE_CONSTANT, 
                                           "type", GIL_FLOAT, 
                                           NULL);  
    gil_constant_set_float(constant, 1.5); 
    ct_test(test, GIL_FLOAT == gil_constant_get_const_type(constant));
    ct_test(test, 1.5 == gil_constant_get_float(constant));
    g_object_unref(constant);
  }
}

static void
constant_setup(Test *test)
{
}

static void
constant_teardown(Test *test)
{
}

Test *
create_constant_test()
{
  Test *t = ct_create("GilConstantTest");

  g_assert(ct_addSetUp(t, constant_setup));
  g_assert(ct_addTearDown(t, constant_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_constant_g_object_new));
  g_assert(ct_addTestFun(t, test_constant_g_object_get));
  g_assert(ct_addTestFun(t, test_constant_g_object_set));
#endif

  return t;
}
